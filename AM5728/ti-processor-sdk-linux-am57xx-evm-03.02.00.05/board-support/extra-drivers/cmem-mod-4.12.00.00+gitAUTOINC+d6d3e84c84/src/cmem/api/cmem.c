/*
 * Copyright (c) 2007-2014 Texas Instruments Incorporated - http://www.ti.com
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/*
 * cmem.c
 */

#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

#include <ti/cmem.h>

CMEM_AllocParams CMEM_DEFAULTPARAMS = {
    CMEM_POOL,		/* type */
    CMEM_NONCACHED,	/* flags */
    1			/* alignment */
};

#ifdef __DEBUG
#define __D(fmt, args...) fprintf(stderr, "CMEM Debug: " fmt, ## args)
#else
#define __D(fmt, args...)
#endif

#define __E(fmt, args...) fprintf(stderr, "CMEM Error: " fmt, ## args)

struct block_struct {
    void *addr;
    size_t size;
};

/* set cmem_fd to -2 to distinguish from failed open (-1) */
static int cmem_fd = -2;
static int ref_count = 0;

static void *getAndAllocFromPool(int blockid, unsigned long long size, CMEM_AllocParams *params, off_t *physp);
static void *allocFromHeap(int blockid, size_t size, CMEM_AllocParams *params, off_t *physp);
static void *allocFromPool(int blockid, int poolid, CMEM_AllocParams *params, off_t *physp);
static void *allocPool(int blockid, int poolid, CMEM_AllocParams *params, off_t *physp);

static int validate_init()
{
    switch (cmem_fd) {
      case -3:
	__E("CMEM_exit() already called, check stderr output for earlier "
	    "CMEM failure messages (possibly version mismatch).\n");

	return 0;

      case -2:
        __E("CMEM_init() not called, you must initialize CMEM before "
	    "making API calls.\n");

	return 0;

      case -1:
        __E("CMEM file descriptor -1 (failed 'open()'), ensure CMEMK "
	    "kernel module cmemk.ko has been installed with 'insmod'");

	return 0;

      default:
	return 1;
    }
}

int CMEM_init(void)
{
    int flags;
    unsigned int version;

    __D("init: entered - ref_count %d, cmem_fd %d\n", ref_count, cmem_fd);

    if (cmem_fd >= 0) {
	ref_count++;
	__D("init: /dev/cmem already opened, incremented ref_count %d\n",
            ref_count);
	return 0;
    }

    cmem_fd = open("/dev/cmem", O_RDWR);

    if (cmem_fd == -1) {
        __E("init: Failed to open /dev/cmem: '%s'\n", strerror(errno));
        return -1;
    }

    ref_count++;

    __D("init: successfully opened /dev/cmem, matching driver version...\n");

    version = CMEM_getVersion();
    if ((version & 0xffff0000) != (CMEM_VERSION & 0xffff0000)) {
        __E("init: major version mismatch between interface and driver.\n");
        __E("    needs driver version %#x, got %#x\n", CMEM_VERSION, version);
	CMEM_exit();
	return -1;
    }
    else if ((version & 0x0000ffff) < (CMEM_VERSION & 0x0000ffff)) {
        __E("init: minor version mismatch between interface and driver.\n");
        __E("    needs driver minor version %#x or greater.\n"
	    "    got minor version %#x (full version %#x)\n",
	    CMEM_VERSION & 0x0000ffff, version & 0x0000ffff, version);
	CMEM_exit();
	return -1;
    }

    __D("init: ... match good (%#x)\n", version);

    flags = fcntl(cmem_fd, F_GETFD);
    if (flags != -1) {
        fcntl(cmem_fd, F_SETFD, flags | FD_CLOEXEC);
    }
    else {
        __E("init: fcntl(F_GETFD) failed: '%s'\n", strerror(errno));
    }

    __D("init: exiting, returning success\n");

    return 0;
}

static void *allocFromPool(int blockid, int poolid, CMEM_AllocParams *params, off_t *physp)
{
    union CMEM_AllocUnion allocDesc;
#if defined(LINUXUTILS_BUILDOS_ANDROID)
    off64_t phys;
#else
    off_t phys;
#endif
    void *userp;
    unsigned long long size;
    size_t map_size;
    unsigned int cmd;
    int rv;

    allocDesc.alloc_pool_inparams.poolid = poolid;
    allocDesc.alloc_pool_inparams.blockid = blockid;
    cmd = CMEM_IOCALLOC | params->flags;
    rv = ioctl(cmem_fd, cmd | CMEM_IOCMAGIC, &allocDesc);
    if (rv != 0) {
        __E("allocPool: ioctl %s failed from pool %d: %d\n",
	    cmd == CMEM_IOCALLOC ? "CMEM_IOCALLOC" : "CMEM_IOCALLOCCACHED",
	    poolid, rv);
	if (physp) {
	    *physp = (unsigned int)NULL;
	}

        return NULL;
    }
#if defined(LINUXUTILS_BUILDOS_ANDROID)
    phys = (off64_t)allocDesc.alloc_pool_outparams.physp;
#else
    phys = (off_t)allocDesc.alloc_pool_outparams.physp;
#endif
    size = allocDesc.alloc_pool_outparams.size;

    __D("allocPool: allocated phys buffer %#llx, size %#llx\n",
        (unsigned long long)phys, size);

    /* non-NULL physp means "don't map", return NULL since no virt ptr */
    if (physp) {
#if defined(LINUXUTILS_BUILDOS_ANDROID)
	*physp = (off_t)phys;
#else
	*physp = phys;
#endif
	return NULL;
    }

    map_size = (size_t)size;
    if (map_size != size) {
        __E("allocPool: returned pool buffer too big for mmap (size %#llx)\n",
            size);
        __E("    Freeing phys buffer %#llx\n", (unsigned long long)phys);
        ioctl(cmem_fd, CMEM_IOCFREEPHYS | CMEM_IOCMAGIC , &phys);
        return NULL;
    }

    /* Map the physical address to user space */
#if defined(LINUXUTILS_BUILDOS_ANDROID)
    userp = mmap64(0,                       // Preferred start address
                   map_size,                // Length to be mapped
                   PROT_WRITE | PROT_READ,  // Read and write access
                   MAP_SHARED,              // Shared memory
                   cmem_fd,                 // File descriptor
                   phys);                   // The byte offset from fd

    if (userp == MAP_FAILED) {
        __E("allocPool: Failed to mmap64 buffer at physical address %#llx\n",
            (unsigned long long)phys);
        __E("    Freeing phys buffer %#llx\n", (unsigned long long)phys);
        ioctl(cmem_fd, CMEM_IOCFREEPHYS | CMEM_IOCMAGIC , &phys);
        return NULL;
    }

    __D("allocPool: mmap64 succeeded, returning virt buffer %p\n", userp);
#else
    userp = mmap(0,                       // Preferred start address
                 map_size,                // Length to be mapped
                 PROT_WRITE | PROT_READ,  // Read and write access
                 MAP_SHARED,              // Shared memory
                 cmem_fd,                 // File descriptor
                 phys);                   // The byte offset from fd

    if (userp == MAP_FAILED) {
        __E("allocPool: Failed to mmap buffer at physical address %#llx\n",
            (unsigned long long)phys);
        __E("    Freeing phys buffer %#llx\n", (unsigned long long)phys);
        ioctl(cmem_fd, CMEM_IOCFREEPHYS | CMEM_IOCMAGIC , &phys);
        return NULL;
    }

    __D("allocPool: mmap succeeded, returning virt buffer %p\n", userp);
#endif

    return userp;
}

static void *getAndAllocFromPool(int blockid, unsigned long long size, CMEM_AllocParams *params, off_t *physp)
{
    int poolid;

    poolid = CMEM_getPool2(blockid, size);

    if (poolid == -1) {
	if (physp) {
	    *physp = (unsigned int)NULL;
	}

	return NULL;
    }
    if (poolid == -2) {
	return allocFromHeap(blockid, (size_t)size, params, physp);
    }
    else {
	return allocFromPool(blockid, poolid, params, physp);
    }
}

static void *allocFromHeap(int blockid, size_t size, CMEM_AllocParams *params, off_t *physp)
{
    void *userp;
    union CMEM_AllocUnion allocDesc;
#if defined(LINUXUTILS_BUILDOS_ANDROID)
    off64_t phys;
#else
    off_t phys;
#endif
    unsigned int cmd;
    int rv;

    cmd = CMEM_IOCALLOCHEAP | params->flags;
    allocDesc.alloc_heap_inparams.size = size;
    allocDesc.alloc_heap_inparams.align = params->alignment == 0 ?
                                  1 : params->alignment;
    allocDesc.alloc_heap_inparams.blockid = blockid;

    rv = ioctl(cmem_fd, cmd | CMEM_IOCMAGIC, &allocDesc);
    if (rv != 0) {
        __E("allocHeap: ioctl %s failed: %d\n",
	    cmd == CMEM_IOCALLOCHEAP ?
	           "CMEM_IOCALLOCHEAP" : "CMEM_IOCALLOCHEAPCACHED",
	    rv);
	if (physp) {
	    *physp = (unsigned int)NULL;
	}

        return NULL;
    }
#if defined(LINUXUTILS_BUILDOS_ANDROID)
    phys = (off64_t)allocDesc.physp;
#else
    phys = (off_t)allocDesc.physp;
#endif

    __D("allocHeap: allocated phys buffer %#llx\n", (unsigned long long)phys);

    /* non-NULL physp means "don't map", return NULL since no virt ptr */
    if (physp) {
#if defined(LINUXUTILS_BUILDOS_ANDROID)
	*physp = (off_t)phys;
#else
	*physp = phys;
#endif
	return NULL;
    }

    /* Map the physical address to user space */
#if defined(LINUXUTILS_BUILDOS_ANDROID)
    userp = mmap64(0,                       // Preferred start address
                   size,                    // Length to be mapped
                   PROT_WRITE | PROT_READ,  // Read and write access
                   MAP_SHARED,              // Shared memory
                   cmem_fd,                 // File descriptor
                   phys);                   // The byte offset from fd

    if (userp == MAP_FAILED) {
        __E("allocHeap: Failed to mmap64 buffer at physical address %#llx\n",
            (unsigned long long)phys);
        __E("    Freeing phys buffer %#llx\n", (unsigned long long)phys);
        ioctl(cmem_fd, CMEM_IOCFREEHEAPPHYS | CMEM_IOCMAGIC, &phys);
        return NULL;
    }

    __D("allocHeap: mmap64 succeeded, returning virt buffer %p\n", userp);
#else
    userp = mmap(0,                       // Preferred start address
                 size,                    // Length to be mapped
                 PROT_WRITE | PROT_READ,  // Read and write access
                 MAP_SHARED,              // Shared memory
                 cmem_fd,                 // File descriptor
                 phys);                   // The byte offset from fd

    if (userp == MAP_FAILED) {
        __E("allocHeap: Failed to mmap buffer at physical address %#llx\n",
            (unsigned long long)phys);
        __E("    Freeing phys buffer %#llx\n", (unsigned long long)phys);
        ioctl(cmem_fd, CMEM_IOCFREEHEAPPHYS | CMEM_IOCMAGIC, &phys);
        return NULL;
    }

    __D("allocHeap: mmap succeeded, returning virt buffer %p\n", userp);
#endif

    return userp;
}

/*
 *  Single interface into all flavors of alloc.
 *  Need to support:
 *    - cached vs. noncached
 *    - heap vs. pool
 *    - alignment w/ heap allocs
 */
static void *alloc(int blockid, size_t size, CMEM_AllocParams *params, off_t *physp)
{
    if (params == NULL) {
	params = &CMEM_DEFAULTPARAMS;
    }

    __D("alloc: entered w/ size %#x, params - type %s, flags %s, align %#x%s\n",
        size,
        params->type == CMEM_POOL ? "POOL" : "HEAP",
        params->flags & CMEM_CACHED ? "CACHED" : "NONCACHED",
        params->alignment,
        params == &CMEM_DEFAULTPARAMS ? " (default)" : "");

    if (!validate_init()) {
	if (physp) {
	    *physp = (unsigned int)NULL;
	}

	return NULL;
    }

    if (params->type == CMEM_POOL) {
	return getAndAllocFromPool(blockid, (unsigned long long)size, params, physp);
    }
    else {
	return allocFromHeap(blockid, size, params, physp);
    }
}

void *CMEM_alloc(size_t size, CMEM_AllocParams *params)
{
    return alloc(0, size, params, NULL);
}

void *CMEM_alloc2(int blockid, size_t size, CMEM_AllocParams *params)
{
    return alloc(blockid, size, params, NULL);
}

off_t CMEM_allocPhys(size_t size, CMEM_AllocParams *params)
{
    off_t physp;

    alloc(0, size, params, &physp);

    return physp;
}

off_t CMEM_allocPhys2(int blockid, size_t size, CMEM_AllocParams *params)
{
    off_t physp;

    alloc(blockid, size, params, &physp);

    return physp;
}

#if defined(LINUXUTILS_BUILDOS_ANDROID)

off64_t CMEM_allocPhys64(int blockid, size_t size, CMEM_AllocParams *params)
{
    off_t physp;
    off64_t physp64;

    alloc(blockid, size, params, &physp);

    physp64 = (off64_t)physp;

    /* squash the sign-extension when casting to 64-bits */
    physp64 &= 0x00000000ffffffffULL;

    return physp64;
}

off64_t CMEM_allocPoolPhys64(int blockid, int poolid, CMEM_AllocParams *params)
{
    off_t physp;
    off64_t physp64;

    allocPool(blockid, poolid, params, &physp);

    physp64 = (off64_t)physp;

    /* squash the sign-extension when casting to 64-bits */
    physp64 &= 0x00000000ffffffffULL;

    return physp64;
}

void *CMEM_registerAlloc64(off64_t physp)
{
    union CMEM_AllocUnion allocDesc;
    void *userp;
    size_t size;
    int rv;

    allocDesc.physp = (unsigned long long)physp;
    rv = ioctl(cmem_fd, CMEM_IOCREGUSER | CMEM_IOCMAGIC, &allocDesc);
    if (rv != 0) {
        __E("registerAlloc64: ioctl CMEM_IOCREGUSER failed for phys addr %#llx: %d\n",
	    (unsigned long long)physp, rv);

        return NULL;
    }
    size = allocDesc.size;

    __D("registerAlloc64: registered use of phys buffer %#llx, size %#x\n",
        (unsigned long long)physp, size);

    /* Map the physical address to user space */
    userp = mmap64(0,                       // Preferred start address
                   size,                    // Length to be mapped
                   PROT_WRITE | PROT_READ,  // Read and write access
                   MAP_SHARED,              // Shared memory
                   cmem_fd,                 // File descriptor
                   physp);                  // The byte offset from fd

    if (userp == MAP_FAILED) {
        __E("registerAlloc64: Failed to mmap64 buffer at physical address %#llx\n",
            (unsigned long long)physp);
        __E("    Unregistering use of phys buffer %#llx\n",
	    (unsigned long long)physp);
        ioctl(cmem_fd, CMEM_IOCFREEPHYS | CMEM_IOCMAGIC, &physp);

        return NULL;
    }

    __D("registerAlloc64: mmap64 succeeded, returning virt buffer %p\n", userp);

    return userp;
}

void *CMEM_map64(off64_t physp, size_t size)
{
    void *userp;

    __D("map64: mapping phys buffer %#llx, size %#x\n",
        (unsigned long long)physp, size);

    /* Map the physical address to user space */
    userp = mmap64(0,                       // Preferred start address
                   size,                    // Length to be mapped
                   PROT_WRITE | PROT_READ,  // Read and write access
                   MAP_SHARED,              // Shared memory
                   cmem_fd,                 // File descriptor
                   physp);                  // The byte offset from fd

    if (userp == MAP_FAILED) {
        __E("map64: Failed to mmap64 buffer at physical address %#llx\n",
            (unsigned long long)physp);

        return NULL;
    }

    __D("map64: mmap64 succeeded, returning virt buffer 0x%p\n", userp);

    return userp;
}

off64_t CMEM_getPhys64(void *ptr)
{
    union CMEM_AllocUnion getDesc;

    __D("getPhys64: entered w/ addr %p\n", ptr);

    if (!validate_init()) {
	return 0;
    }

    getDesc.virtp = ptr;
    if (ioctl(cmem_fd, CMEM_IOCGETPHYS | CMEM_IOCMAGIC, &getDesc) == -1) {
        __E("getPhys64: Failed to get physical address of %#x\n",
            (unsigned int)ptr);
        return 0;
    }

    __D("getPhys64: exiting, ioctl CMEM_IOCGETPHYS succeeded, returning %#llx\n",
        getDesc.physp);

    return (off64_t)getDesc.physp;
}

int CMEM_freePhys64(off64_t physp, CMEM_AllocParams *params)
{
    if (params == NULL) {
	params = &CMEM_DEFAULTPARAMS;
    }

    __D("freePhys64: entered w/ physical address %#llx, params - type %s%s\n",
        (unsigned long long)physp,
        params->type == CMEM_POOL ? "POOL" : "HEAP",
        params == &CMEM_DEFAULTPARAMS ? " (default)" : "");

    if (!validate_init()) {
	return -1;
    }

    if (ioctl(cmem_fd, CMEM_IOCFREEPHYS | CMEM_IOCMAGIC, &physp) == -1) {
	__E("freePhys64: Failed to free buffer at physical address %#llx\n",
	    (unsigned long long)physp);

	return -1;
    }

    return 0;
}

#endif

void *CMEM_registerAlloc(off_t physp)
{
    union CMEM_AllocUnion allocDesc;
    void *userp;
    size_t size;
    int rv;

    allocDesc.physp = (unsigned long long)physp;
    rv = ioctl(cmem_fd, CMEM_IOCREGUSER | CMEM_IOCMAGIC, &allocDesc);
    if (rv != 0) {
        __E("registerAlloc: ioctl CMEM_IOCREGUSER failed for phys addr %#llx: %d\n",
	    (unsigned long long)physp, rv);

        return NULL;
    }
    size = allocDesc.size;

    __D("registerAlloc: registered use of phys buffer %#llx, size %#x\n",
        (unsigned long long)physp, size);

    /* Map the physical address to user space */
    userp = mmap(0,                       // Preferred start address
                 size,                    // Length to be mapped
                 PROT_WRITE | PROT_READ,  // Read and write access
                 MAP_SHARED,              // Shared memory
                 cmem_fd,                 // File descriptor
                 physp);                  // The byte offset from fd

    if (userp == MAP_FAILED) {
        __E("registerAlloc: Failed to mmap buffer at physical address %#llx\n",
            (unsigned long long)physp);
        __E("    Unregistering use of phys buffer %#llx\n",
	    (unsigned long long)physp);
        ioctl(cmem_fd, CMEM_IOCFREEPHYS | CMEM_IOCMAGIC, &physp);

        return NULL;
    }

    __D("registerAlloc: mmap succeeded, returning virt buffer %p\n", userp);

    return userp;
}

void *CMEM_map(off_t physp, size_t size)
{
    void *userp;

    __D("map: mapping phys buffer %#llx, size %#x\n",
        (unsigned long long)physp, size);

    /* Map the physical address to user space */
    userp = mmap(0,                       // Preferred start address
                 size,                    // Length to be mapped
                 PROT_WRITE | PROT_READ,  // Read and write access
                 MAP_SHARED,              // Shared memory
                 cmem_fd,                 // File descriptor
                 physp);                  // The byte offset from fd

    if (userp == MAP_FAILED) {
        __E("map: Failed to mmap buffer at physical address %#llx\n",
            (unsigned long long)physp);

        return NULL;
    }

    __D("map: mmap succeeded, returning virt buffer 0x%p\n", userp);

    return userp;
}

int CMEM_unmap(void *userp, size_t size)
{
    if (munmap(userp, size) == -1) {
        __E("unmap: failed to munmap 0x%p\n", userp);
        return -1;
    }

    __D("unmap: munmap succeeded, returning 0\n");

    return 0;
}

static void *allocPool(int blockid, int poolid, CMEM_AllocParams *params, off_t *physp)
{
    if (params == NULL) {
	params = &CMEM_DEFAULTPARAMS;
    }

    __D("allocPool: entered w/ poolid %d, params - flags %s%s\n", poolid,
        params->flags & CMEM_CACHED ? "CACHED" : "NONCACHED",
        params == &CMEM_DEFAULTPARAMS ? " (default)" : "");

    if (!validate_init()) {
	if (physp) {
	    *physp = (unsigned int)NULL;
	}

	return NULL;
    }

    return allocFromPool(blockid, poolid, params, physp);
}

void *CMEM_allocPool(int poolid, CMEM_AllocParams *params)
{
    return allocPool(0, poolid, params, NULL);
}

void *CMEM_allocPool2(int blockid, int poolid, CMEM_AllocParams *params)
{
    return allocPool(blockid, poolid, params, NULL);
}

off_t CMEM_allocPoolPhys(int poolid, CMEM_AllocParams *params)
{
    off_t physp;

    allocPool(0, poolid, params, &physp);

    return physp;
}

off_t CMEM_allocPoolPhys2(int blockid, int poolid, CMEM_AllocParams *params)
{
    off_t physp;

    allocPool(blockid, poolid, params, &physp);

    return physp;
}

int CMEM_free(void *ptr, CMEM_AllocParams *params)
{
    union CMEM_AllocUnion freeDesc;
    unsigned int cmd;
    size_t size;

    if (params == NULL) {
	params = &CMEM_DEFAULTPARAMS;
    }

    __D("free: entered w/ ptr %p, params - type %s%s\n",
        ptr,
        params->type == CMEM_POOL ? "POOL" : "HEAP",
        params == &CMEM_DEFAULTPARAMS ? " (default)" : "");

    if (!validate_init()) {
	return -1;
    }

    freeDesc.virtp = ptr;
    cmd = CMEM_IOCFREE | params->type;
    if (ioctl(cmem_fd, cmd | CMEM_IOCMAGIC, &freeDesc) == -1) {
        __E("free: failed to free %#x\n", (unsigned int) ptr);
        return -1;
    }
    size = freeDesc.free_outparams.size;

    __D("free: ioctl CMEM_IOCFREE%s succeeded, size %#x\n",
        params->type == CMEM_POOL ? "POOL" : "HEAP", size);

    if (munmap(ptr, size) == -1) {
        __E("free: failed to munmap %#x\n", (unsigned int) ptr);
        return -1;
    }

    __D("free: munmap succeeded, returning 0\n");

    return 0;
}

int CMEM_freePhys(off_t physp, CMEM_AllocParams *params)
{
    if (params == NULL) {
	params = &CMEM_DEFAULTPARAMS;
    }

    __D("freePhys: entered w/ physical address %#llx, params - type %s%s\n",
        (unsigned long long)physp,
        params->type == CMEM_POOL ? "POOL" : "HEAP",
        params == &CMEM_DEFAULTPARAMS ? " (default)" : "");

    if (!validate_init()) {
	return -1;
    }

    if (ioctl(cmem_fd, CMEM_IOCFREEPHYS | CMEM_IOCMAGIC, &physp) == -1) {
	__E("freePhys: Failed to free buffer at physical address %#llx\n",
	    (unsigned long long)physp);

	return -1;
    }

    return 0;
}

int CMEM_unregister(void *ptr, CMEM_AllocParams *params)
{
    __D("unregister: delegating to CMEM_free()...\n");

    return CMEM_free(ptr, params);
}

static int getPoolFromBlock(int blockid, unsigned long long size)
{
    union CMEM_AllocUnion poolDesc;

    if (!validate_init()) {
	return -1;
    }

    poolDesc.get_pool_inparams.size = size;
    poolDesc.get_pool_inparams.blockid = blockid;
    if (ioctl(cmem_fd, CMEM_IOCGETPOOL | CMEM_IOCMAGIC, &poolDesc) == -1) {
        __E("getPool: Failed to get a pool fitting a size %#llx\n", size);
        return -1;
    }

    __D("getPool: exiting, ioctl CMEM_IOCGETPOOL succeeded, returning %d\n",
        poolDesc.poolid);

    return poolDesc.poolid;
}

int CMEM_getPool(unsigned long long size)
{
    __D("getPool: entered w/ size %#llx\n", size);

    return getPoolFromBlock(0, size);
}

int CMEM_getPool2(int blockid, unsigned long long size)
{
    __D("getPool2: entered w/ size %#llx\n", size);

    return getPoolFromBlock(blockid, size);
}

off_t CMEM_getPhys(void *ptr)
{
    union CMEM_AllocUnion getDesc;

    __D("getPhys: entered w/ addr %p\n", ptr);

    if (!validate_init()) {
	return 0;
    }

    getDesc.virtp = ptr;
    if (ioctl(cmem_fd, CMEM_IOCGETPHYS | CMEM_IOCMAGIC, &getDesc) == -1) {
        __E("getPhys: Failed to get physical address of %#x\n",
            (unsigned int)ptr);
        return 0;
    }

    __D("getPhys: exiting, ioctl CMEM_IOCGETPHYS succeeded, returning %#llx\n",
        getDesc.physp);

    return (off_t)getDesc.physp;
}

int CMEM_cacheWbInvAll(void)
{
    __D("cacheWbInvAll: entered\n");

    if (!validate_init()) {
	return -1;
    }

    if (ioctl(cmem_fd, CMEM_IOCCACHEWBINVALL | CMEM_IOCMAGIC, NULL) == -1) {
        __E("cacheWbInvAll: Failed to writeback/invalidate all\n");

        return -1;
    }

    __D("cacheWbInvAll: exiting, ioctl CMEM_IOCCACHEWBINVALL succeeded, returning 0\n");

    return 0;
}

int CMEM_cacheWb(void *ptr, size_t size)
{
    struct block_struct block;

    __D("cacheWb: entered w/ addr %p, size %#x\n", ptr, size);

    if (!validate_init()) {
	return -1;
    }

    block.addr = ptr;
    block.size = size;
    if (ioctl(cmem_fd, CMEM_IOCCACHEWB | CMEM_IOCMAGIC, &block) == -1) {
        __E("cacheWb: Failed to writeback %#x\n", (unsigned int) ptr);

        return -1;
    }

    __D("cacheWb: exiting, ioctl CMEM_IOCCACHEWB succeeded, returning 0\n");

    return 0;
}

int CMEM_cacheWbInv(void *ptr, size_t size)
{
    struct block_struct block;

    __D("cacheWbInv: entered w/ addr %p, size %#x\n", ptr, size);

    if (!validate_init()) {
	return -1;
    }

    block.addr = ptr;
    block.size = size;
    if (ioctl(cmem_fd, CMEM_IOCCACHEWBINV | CMEM_IOCMAGIC, &block) == -1) {
        __E("cacheWbInv: Failed to writeback & invalidate %#x\n",
            (unsigned int) ptr);

        return -1;
    }

    __D("cacheWbInv: exiting, ioctl CMEM_IOCCACHEWBINV succeeded, returning 0\n");

    return 0;
}

int CMEM_cacheInv(void *ptr, size_t size)
{
    struct block_struct block;

    __D("cacheInv: entered w/ addr %p, size %#x\n", ptr, size);

    if (!validate_init()) {
	return -1;
    }

    block.addr = ptr;
    block.size = size;
    if (ioctl(cmem_fd, CMEM_IOCCACHEINV | CMEM_IOCMAGIC, &block) == -1) {
        __E("cacheInv: Failed to invalidate %#x\n", (unsigned int) ptr);

        return -1;
    }

    __D("cacheInv: exiting, ioctl CMEM_IOCCACHEINV succeeded, returning 0\n");

    return 0;
}

int CMEM_getVersion(void)
{
    unsigned int version;
    int rv;

    __D("getVersion: entered\n");

    if (!validate_init()) {
	return -1;
    }

    rv = ioctl(cmem_fd, CMEM_IOCGETVERSION | CMEM_IOCMAGIC, &version);
    if (rv != 0) {
	__E("getVersion: Failed to retrieve version from driver: %d.\n", rv);

	return -1;
    }

    __D("getVersion: exiting, ioctl CMEM_IOCGETVERSION returned %#x\n",
        version);

    return version;
}

static int getBlock(int blockid, off_t *pphys_base, unsigned long long *psize)
{
    union CMEM_AllocUnion block;
    int rv;

    __D("getBlock: entered\n");

    if (!validate_init()) {
	return -1;
    }

    block.blockid = blockid;
    rv = ioctl(cmem_fd, CMEM_IOCGETBLOCK | CMEM_IOCMAGIC, &block);
    if (rv != 0) {
	__E("getBlock: Failed to retrieve memory block bounds for block %d "
            "from driver: %d.\n", blockid, rv);

	return -1;
    }

    *pphys_base = (off_t)block.get_block_outparams.physp;
    *psize = block.get_block_outparams.size;

    __D("getBlock: exiting, ioctl CMEM_IOCGETBLOCK succeeded, "
        "returning *pphys_base=%#llx, *psize=%#llx\n",
        (unsigned long long)(*pphys_base), *psize);

    return 0;
}

int CMEM_getBlock(off_t *pphys_base, unsigned long long *psize)
{
    return getBlock(0, pphys_base, psize);
}

int CMEM_getBlockAttrs(int blockid, CMEM_BlockAttrs *pattrs)
{
    return getBlock(blockid, &pattrs->phys_base, &pattrs->size);
}

int CMEM_getNumBlocks(int *pnblocks)
{
    int rv;

    __D("getNumBlocks: entered\n");

    if (!validate_init()) {
	return -1;
    }

    rv = ioctl(cmem_fd, CMEM_IOCGETNUMBLOCKS | CMEM_IOCMAGIC, pnblocks);
    if (rv != 0) {
	__E("getNumBlocks: Failed to retrieve number of blocks "
            "from driver: %d.\n", rv);

	return -1;
    }

    __D("getNumBlocks: exiting, ioctl CMEM_IOCGETNUMBLOCKS succeeded, "
        "returning *pnblocks=%d\n", *pnblocks);

    return 0;
}

int CMEM_exit(void)
{
    int result = 0;

    __D("exit: entered - ref_count %d, cmem_fd %d\n", ref_count, cmem_fd);

    if (!validate_init()) {
	return -1;
    }

    __D("exit: decrementing ref_count\n");

    ref_count--;
    if (ref_count == 0) {
	result = close(cmem_fd);

	__D("exit: ref_count == 0, closed /dev/cmem (%s)\n",
            result == -1 ? strerror(errno) : "succeeded");

	/* setting -3 allows to distinguish CMEM exit from CMEM failed */
	cmem_fd = -3;
    }

    __D("exit: exiting, returning %d\n", result);

    return result;
}
