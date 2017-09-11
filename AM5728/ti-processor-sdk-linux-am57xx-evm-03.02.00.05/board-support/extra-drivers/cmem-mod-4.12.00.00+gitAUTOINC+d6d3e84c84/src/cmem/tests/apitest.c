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
 * apitest.c
 *
 * Tests basic API usage and memory allocation. Inserting CMEM with the
 * following options works on the DVEVM (if mem=120M):
 *
 * insmod cmemk.ko phys_start=0x87800000 phys_end=0x87F00000 pools=4xBUFSIZE phys_start_1=0x87F00000 phys_end_1=0x88000000 pools_1=4xBUFSIZE
 *
 * where BUFSIZE is the number of bytes you plan on passing as command
 * line parameter to apitest. If in doubt, use a larger number as BUFSIZE
 * denotes the maximum buffer you can allocate.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <errno.h>

#include <ti/cmem.h>

#define NUMHEAPPTRS 0x1000
#define FALSE 	0
#define TRUE	1

unsigned int *ptrs[NUMHEAPPTRS];
int nblocks;
unsigned int non_interactive_flag;

int writereadCMA(size_t size)
{
    int rv;
    unsigned int i;
    CMEM_AllocParams params;
    char *heap_ptr;

    printf("allocating write-read heap buffer from CMA memory block...\n");
    params.type = CMEM_HEAP;
    params.flags = CMEM_NONCACHED;
    params.alignment = 0;
    heap_ptr = CMEM_alloc2(CMEM_CMABLOCKID, size, &params);
    if (heap_ptr == NULL) {
	printf("...failed\n");
	return -1;
    }
    else {
	printf("...done, allocated buffer at virtp %p\n", heap_ptr);
    }

    printf("Performing write-read test on CMA buffer...\n");
    for (i = 0; i < size; i++) {
	heap_ptr[i] = i % 256;
    }
    for (i = 0; i < size; i++) {
	if (heap_ptr[i] != (i % 256)) {
	    printf("write-read error: read 0x%x, should be 0x%x\n",
	           heap_ptr[i], i % 256);
	    if (non_interactive_flag == FALSE) {
		printf("Press ENTER to continue\n");
		getchar();
	    }
	    break;
	}
    }
    if (i == size) {
	if (non_interactive_flag == FALSE) {
	    printf("...succeeded, press ENTER to continue\n");
	    getchar();
	}	    
	else {
	    printf("...succeeded\n");
	}
    }

    printf("calling CMEM_cacheInv(heap_ptr, size)...\n");
    CMEM_cacheInv(heap_ptr, size);
    printf("...done\n");

    printf("freeing heap buffer...\n");
    rv = CMEM_free(heap_ptr, &params);
    if (rv < 0) {
	printf("error freeing buffer\n");
    }
    printf("...done\n");

    return 0;
}

void testCMA(size_t size)
{
    int rv;
    int num_buffers;
    int i;
    CMEM_AllocParams params;

    printf("allocating heap buffers from CMA memory block...\n");
    printf("    (will end with expected failed allocation)\n");

    num_buffers = 0;
    params.type = CMEM_HEAP;
    params.flags = CMEM_NONCACHED;
    params.alignment = 0;
    while (num_buffers < NUMHEAPPTRS) {
	ptrs[num_buffers] = CMEM_alloc2(CMEM_CMABLOCKID, size, &params);
	if (ptrs[num_buffers] == NULL) {
	    break;
	}
	num_buffers++;
    }
    printf("...done, %d allocated\n", num_buffers);

    if (non_interactive_flag == FALSE) {
	printf("Press ENTER to continue\n");
	getchar();
    }

    printf("freeing heap blocks...\n");
    for (i = 0; i < num_buffers; i++) {
	rv = CMEM_free(ptrs[i], &params);
	if (rv < 0) {
	    printf("error freeing blocks\n");
	    break;
	}
	ptrs[i] = NULL;
    }
    printf("...done\n");

    /* make sure we can still allocate num_buffers after freeing */
    printf("allocating %d heap blocks...\n", num_buffers);
    for (i = 0; i < num_buffers; i++) {
	ptrs[i] = CMEM_alloc2(CMEM_CMABLOCKID, size, &params);
	if (ptrs[i] == NULL) {
	    printf("error re-allocating %d heap blocks\n", num_buffers);
	    break;
	}
    }
    printf("...done, freeing heap blocks...\n");
    for (i = 0; i < num_buffers; i++) {
	rv = CMEM_free(ptrs[i], &params);
	if (rv < 0) {
	    printf("error freeing blocks\n");
	    break;
	}
    }
    printf("...done\n");

    if (num_buffers) {
	writereadCMA(size);
    }

}

void testHeap(size_t size, int block)
{
    int rv;
    int num_buffers;
    int i;
    CMEM_AllocParams params;

    printf("allocating heap buffers from CMEM memory block %d...\n", block);
    printf("    (will end with expected failed allocation)\n");

    num_buffers = 0;
    params.type = CMEM_HEAP;
    params.flags = CMEM_NONCACHED;
    params.alignment = 0;
    while (num_buffers < NUMHEAPPTRS) {
	ptrs[num_buffers] = CMEM_alloc2(block, size, &params);
	if (ptrs[num_buffers] == NULL) {
	    break;
	}
	num_buffers++;
    }
    printf("...done, %d allocated\n", num_buffers);

    if (non_interactive_flag == FALSE) {
	printf("Press ENTER to continue\n");
	getchar();
    }	

    printf("freeing heap blocks...\n");
    for (i = 0; i < num_buffers; i++) {
	rv = CMEM_free(ptrs[i], &params);
	if (rv < 0) {
	    printf("error freeing blocks\n");
	    break;
	}
	ptrs[i] = NULL;
    }

    /* make sure we can still allocate num_buffers after freeing */
    printf("allocating %d heap blocks...\n", num_buffers);
    for (i = 0; i < num_buffers; i++) {
	ptrs[i] = CMEM_alloc2(block, size, &params);
	if (ptrs[i] == NULL) {
	    printf("error re-allocating %d heap blocks\n", num_buffers);
	    break;
	}
    }
    printf("...done, freeing heap blocks...\n");
    for (i = 0; i < num_buffers; i++) {
	rv = CMEM_free(ptrs[i], &params);
	if (rv < 0) {
	    printf("error freeing blocks\n");
	    break;
	}
    }
    printf("...done\n");
}

void testPools(size_t size, int block)
{
    int rv;
    int num_buffers;
    int i;
    CMEM_AllocParams params;

    printf("allocating pool buffers from CMEM memory block %d...\n", block);
    printf("    (will end with expected failed allocation)\n");

    num_buffers = 0;
    params.type = CMEM_POOL;
    params.flags = CMEM_NONCACHED;
    params.alignment = 0;
    while (num_buffers < NUMHEAPPTRS) {
	ptrs[num_buffers] = CMEM_alloc2(block, size, &params);
	if (ptrs[num_buffers] == NULL) {
	    break;
	}
	num_buffers++;
    }
    printf("...done, %d allocated\n", num_buffers);

    if (non_interactive_flag == FALSE) {
	printf("Press ENTER to continue\n");
	getchar();
    }

    printf("freeing pool blocks...\n");
    for (i = 0; i < num_buffers; i++) {
	rv = CMEM_free(ptrs[i], &params);
	if (rv < 0) {
	    printf("error freeing blocks\n");
	    break;
	}
	ptrs[i] = NULL;
    }

    /* make sure we can still allocate num_buffers after freeing */
    printf("allocating %d pool blocks...\n", num_buffers);
    for (i = 0; i < num_buffers; i++) {
	ptrs[i] = CMEM_alloc2(block, size, &params);
	if (ptrs[i] == NULL) {
	    printf("error re-allocating %d heap blocks\n", num_buffers);
	    break;
	}
    }
    printf("...done, freeing pool blocks...\n");
    for (i = 0; i < num_buffers; i++) {
	rv = CMEM_free(ptrs[i], &params);
	if (rv < 0) {
	    printf("error freeing blocks\n");
	    break;
	}
    }
    printf("...done\n");
}

void testCache(size_t size, int block)
{
    unsigned int *ptr1_nocache = NULL;
    unsigned int *ptr1_cache = NULL;
    unsigned int *ptr1_dma = NULL;
    unsigned int *ptr2 = NULL;
    off_t physp;
    off_t physp_dma;
    off_t physp_nocache;
    off_t physp_cache;
    int poolid;
    unsigned int i, j;
    struct timeval start_tv, end_tv;
    unsigned long diff;
    int foo, bar;
    CMEM_AllocParams params;

    printf("Allocating first noncached buffer.\n");

    /* First allocate a buffer from the pool that best fits */
    ptr1_nocache = CMEM_alloc2(block, size, NULL);

    if (ptr1_nocache == NULL) {
        fprintf(stderr, "Failed to allocate buffer of size %d\n", size);
        goto cleanup;
    }

    printf("Allocated buffer of size %d at address %#x.\n", size,
           (unsigned int) ptr1_nocache);

    /* Find out and print the physical address of this buffer */
    physp_nocache = CMEM_getPhys(ptr1_nocache);

    if (physp_nocache == 0) {
        fprintf(stderr, "Failed to get physical address of buffer %#x\n",
                (unsigned int) ptr1_nocache);
        goto cleanup;
    }

    printf("Physical address of allocated buffer is %#llx.\n",
           (unsigned long long)physp_nocache);

    /* Write some data into this buffer */
    for (i=0; i < size / sizeof(size_t) ; i++) {
        ptr1_nocache[i] = 0xbeefbeef;
    }

    printf("Allocating first cached buffer.\n");

    /* First allocate a buffer from the pool that best fits */
    params = CMEM_DEFAULTPARAMS;
    params.flags = CMEM_CACHED;
    ptr1_cache = CMEM_alloc2(block, size, &params);

    if (ptr1_cache == NULL) {
        fprintf(stderr, "Failed to allocate buffer of size %d\n", size);
        goto cleanup;
    }

    printf("Allocated buffer of size %d at address %#x.\n", size,
           (unsigned int)ptr1_cache);

    /* Find out and print the physical address of this buffer */
    physp_cache = CMEM_getPhys(ptr1_cache);

    if (physp_cache == 0) {
        fprintf(stderr, "Failed to get physical address of buffer %#x\n",
                (unsigned int)ptr1_cache);
        goto cleanup;
    }

    printf("Physical address of allocated buffer is %#llx.\n",
           (unsigned long long)physp_cache);

    /* Write some data into this buffer */
    for (i = 0; i < size / sizeof(size_t); i++) {
        ptr1_cache[i] = 0x0dead1ce;
    }

    printf("Allocating noncached DMA source buffer.\n");

    /* Allocate a noncached buffer for the DMA source */
    ptr1_dma = CMEM_alloc2(block, size, NULL);

    if (ptr1_dma == NULL) {
        fprintf(stderr, "Failed to allocate buffer of size %d\n", size);
        goto cleanup;
    }

    printf("Allocated buffer of size %d at address %#x.\n", size,
           (unsigned int)ptr1_dma);

    /* Find out and print the physical address of this buffer */
    physp_dma = CMEM_getPhys(ptr1_dma);

    if (physp_dma == 0) {
        fprintf(stderr, "Failed to get physical address of buffer %#x\n",
                (unsigned int)ptr1_dma);
        goto cleanup;
    }

    printf("Physical address of allocated buffer is %#llx.\n",
           (unsigned long long)physp_dma);

    /* Initialize DMA source buffer */
    for (i = 0; i < size / sizeof(size_t); i++) {
        ptr1_cache[i] = 0x0dead1ce;
    }

    /*
     * Measure the write performance of each buffer to check that one
     * is cached and the other isn't.
     */
    printf("Measuring R-M-W performance (cached should be quicker).\n");
    for (j = 0; j < 3; j++) {
	printf("R-M-W noncached buffer %#llx\n",
	       (unsigned long long)physp_nocache);
	gettimeofday(&start_tv, NULL);
	for (i = 0; i < (size / sizeof(size_t)); i += 1) {
	    ptr1_nocache[i] += 1;
	}
	gettimeofday(&end_tv, NULL);
	diff = end_tv.tv_usec - start_tv.tv_usec;
	if (end_tv.tv_sec > start_tv.tv_sec) {
	    diff += (end_tv.tv_sec - start_tv.tv_sec) * 1000000;
	}
	printf("  diff=%ld\n", diff);

	printf("R-M-W cached buffer %#llx\n", (unsigned long long)physp_cache);
	gettimeofday(&start_tv, NULL);
	for (i = 0; i < (size / sizeof(size_t)); i += 1) {
	    ptr1_cache[i] += 1;
	}
	gettimeofday(&end_tv, NULL);
	diff = end_tv.tv_usec - start_tv.tv_usec;
	if (end_tv.tv_sec > start_tv.tv_sec) {
	    diff += (end_tv.tv_sec - start_tv.tv_sec) * 1000000;
	}
	printf("  diff=%ld\n", diff);
    }

    printf("Invalidate cached buffer %p\n", ptr1_cache);

    foo = *ptr1_cache;
    bar = foo;
    bar++;
    *ptr1_cache = bar;
    CMEM_cacheInv(ptr1_cache, size);
    printf("post-flush *ptr1_cache=0x%x\n", foo);
    printf("wrote 0x%x to *ptr1_cache\n", bar);
    printf("post-inv *ptr1_cache=0x%x\n", *ptr1_cache);

    printf("R-M-W cached buffer %#llx\n", (unsigned long long)physp_cache);
    gettimeofday(&start_tv, NULL);
    for (i = 0; i < (size / sizeof(size_t)); i += 1) {
	ptr1_cache[i] += 1;
    }
    gettimeofday(&end_tv, NULL);
    diff = end_tv.tv_usec - start_tv.tv_usec;
    if (end_tv.tv_sec > start_tv.tv_sec) {
	diff += (end_tv.tv_sec - start_tv.tv_sec) * 1000000;
    }
    printf("  diff=%ld\n", diff);

    /*
     * Now allocate another buffer by first finding out which pool that fits
     * best, and then explicitly allocating from that pool. This gives more
     * control at the cost of an extra function call, but essentially does
     * the same thing as the above CMEM_alloc() call.
     */
    printf("Allocating second buffer.\n");

    poolid = CMEM_getPool2(block, size);

    if (poolid == -1) {
        fprintf(stderr, "Failed to get a pool which fits size %d\n", size);
        goto cleanup;
    }

    printf("Got a pool (%d) that fits the size %d\n", poolid, size);

    ptr2 = CMEM_allocPool2(block, poolid, NULL);

    if (ptr2 == NULL) {
        fprintf(stderr, "Failed to allocate buffer of size %d\n", size);
        goto cleanup;
    }

    printf("Allocated buffer of size %d at address %#x.\n", size,
           (unsigned int)ptr2);

    /* Find out and print the physical address of this buffer */
    physp = CMEM_getPhys(ptr2);

    if (physp == 0) {
        fprintf(stderr, "Failed to get physical address of buffer %#x\n",
                (unsigned int)ptr2);
        goto cleanup;
    }

    printf("Physical address of allocated buffer is %#llx.\n",
           (unsigned long long)physp);

    /* Write some data into this buffer */
    for (i=0; i < size / sizeof(size_t); i++) {
        ptr2[i] = 0xfeebfeeb;
    }

    if (non_interactive_flag == FALSE) {
	printf("Inspect your memory map in /proc/%d/maps.\n", getpid());
	printf("Also look at your pool info under /proc/cmem\n");

	printf("Press ENTER to exit (after 'cat /proc/cmem' if desired).\n");
	getchar();
    }

cleanup:
    if (ptr1_nocache != NULL) {
	if (CMEM_free(ptr1_nocache, NULL) < 0) {
	    fprintf(stderr, "Failed to free buffer at %#x\n",
		    (unsigned int)ptr1_nocache);
	}
	printf("Successfully freed buffer at %#x.\n",
	       (unsigned int)ptr1_nocache);
    }

    if (ptr1_cache != NULL) {
	if (CMEM_free(ptr1_cache, &params) < 0) {
	    fprintf(stderr, "Failed to free buffer at %#x\n",
		    (unsigned int)ptr1_cache);
	}
	printf("Successfully freed buffer at %#x.\n",
	       (unsigned int)ptr1_cache);
    }

    if (ptr1_dma != NULL) {
	if (CMEM_free(ptr1_dma, NULL) < 0) {
	    fprintf(stderr, "Failed to free buffer at %#x\n",
		    (unsigned int)ptr1_dma);
	}
	printf("Successfully freed buffer at %#x.\n",
	       (unsigned int)ptr1_dma);
    }

    if (ptr2 != NULL) {
	if (CMEM_free(ptr2, NULL) < 0) {
	    fprintf(stderr, "Failed to free buffer at %#x\n",
		    (unsigned int)ptr2);
	}
	printf("Successfully freed buffer at %#x.\n",
	       (unsigned int)ptr2);
    }
}

int testMap(size_t size)
{
    unsigned int *ptr;
    unsigned int *map_ptr;
#if defined(LINUXUTILS_BUILDOS_ANDROID)
    off64_t physp;
#else
    off_t physp;
#endif
    int ret = 0;

    ptr = CMEM_alloc(size, NULL);
    printf("testMap: ptr = %p\n", ptr);

    if (ptr == NULL) {
	printf("testMap: CMEM_alloc() failed\n");
	return 1;
    }

    printf("    writing 0xdadaface to *ptr\n");
    *ptr = 0xdadaface;

#if defined(LINUXUTILS_BUILDOS_ANDROID)
    physp = CMEM_getPhys64(ptr);
    map_ptr = CMEM_map64(physp, size);
#else
    physp = CMEM_getPhys(ptr);
    map_ptr = CMEM_map(physp, size);
#endif
    if (map_ptr == NULL) {
	printf("testMap: CMEM_map() failed\n");
	ret = 1;
	goto cleanup;
    }
    else {
	printf("testMap: remapped physp %#llx to %p\n",
		physp, map_ptr);
	printf("    *map_ptr = 0x%x\n", *map_ptr);
    }

    if (*map_ptr != 0xdadaface) {
	printf("testMap: failed, read didn't match write\n");
	ret = 1;
    }

    CMEM_unmap(map_ptr, size);

    if (*ptr != 0xdadaface) {
	printf("testMap: after unmap *ptr != 0xdadaface\n");
	ret = 1;
	goto cleanup;
    }
    else {
	printf("testMap: original pointer still good\n");
    }

    printf("testMap: trying to map too much (0x%x)...\n", size * 0x10);
#if defined(LINUXUTILS_BUILDOS_ANDROID)
    map_ptr = CMEM_map64(physp, size * 0x10);
#else
    map_ptr = CMEM_map(physp, size * 0x10);
#endif
    if (map_ptr != NULL) {
	printf("testMap: CMEM_map() should've failed but didn't\n");
	CMEM_unmap(map_ptr, size * 0x10);
	ret = 1;
    }

cleanup:
    CMEM_free(ptr, NULL);

    return ret;
}

int testAllocPhys(size_t size)
{
    unsigned int *map_ptr;
#if defined(LINUXUTILS_BUILDOS_ANDROID)
    off64_t physp;
#else
    off_t physp;
#endif
    int ret = 0;

#if defined(LINUXUTILS_BUILDOS_ANDROID)
    physp = CMEM_allocPhys64(0, size, NULL);
    map_ptr = CMEM_map64(physp, size);
#else
    physp = CMEM_allocPhys(size, NULL);
    map_ptr = CMEM_map(physp, size);
#endif
    if (map_ptr == NULL) {
	printf("testAllocPhys: CMEM_map() failed\n");
	ret = 1;
	goto cleanup;
    }
    else {
	printf("testAllocPhys: mapped physp %#llx to %p\n",
		physp, map_ptr);
    }

    printf("    writing 0xdadaface to *map_ptr\n");
    *map_ptr = 0xdadaface;
    printf("    *map_ptr = 0x%x\n", *map_ptr);

    if (*map_ptr != 0xdadaface) {
	printf("testAllocPhys: failed, read didn't match write\n");
	ret = 1;
    }

    printf("    unmapping %p\n", map_ptr);
    CMEM_unmap(map_ptr, size);

    printf("testAllocPhys: trying to map too much (0x%x)...\n", size * 0x10);
#if defined(LINUXUTILS_BUILDOS_ANDROID)
    map_ptr = CMEM_map64(physp, size * 0x10);
#else
    map_ptr = CMEM_map(physp, size * 0x10);
#endif
    if (map_ptr != NULL) {
	printf("testAllocPhys: CMEM_map() should've failed but didn't\n");
	CMEM_unmap(map_ptr, size * 0x10);
	ret = 1;
    }

cleanup:
#if defined(LINUXUTILS_BUILDOS_ANDROID)
    CMEM_freePhys64(physp, NULL);
#else
    CMEM_freePhys(physp, NULL);
#endif

    return ret;
}

int main(int argc, char *argv[])
{
    size_t size;
    int version;
    CMEM_BlockAttrs attrs;
    int i;
    int c;
    
    non_interactive_flag = FALSE;

    while ((c = getopt(argc, argv, "n")) != -1) {
	switch (c) {
	case 'n':
	    non_interactive_flag = TRUE; 	
	    break;

	default:
	    fprintf(stderr, "Usage: %s [-n] <Number of bytes to allocate>\n",
		    argv[0]);
	    fprintf(stderr,
                    "    -n: non-interactive mode (no ENTER prompts)\n");
	    exit(EXIT_FAILURE);
	}
    }

    if ((argc - optind + 1) != 2) {
	fprintf(stderr, "Usage: %s [-n] <Number of bytes to allocate>\n",
	        argv[0]);
	fprintf(stderr, "    -n: non-interactive mode (no ENTER prompts)\n");
	exit(EXIT_FAILURE);
    }

    errno = 0;
    size = strtol(argv[optind], NULL, 0);

    if (errno) {
	fprintf(stderr, "Bad argument ('%s'), strtol() set errno %d\n",
	        argv[optind], errno);
        exit(EXIT_FAILURE);
    }

    /* First initialize the CMEM module */
    if (CMEM_init() == -1) {
        fprintf(stderr, "Failed to initialize CMEM\n");
        exit(EXIT_FAILURE);
    }

    printf("CMEM initialized.\n");

    version = CMEM_getVersion();
    if (version == -1) {
	fprintf(stderr, "Failed to retrieve CMEM version\n");
        exit(EXIT_FAILURE);
    }
    printf("CMEM version = 0x%x\n", version);

    testMap(size);
    testAllocPhys(size);

    testCMA(size);

    if (CMEM_getNumBlocks(&nblocks)) {
	fprintf(stderr, "Failed to retrieve number of blocks\n");
        exit(EXIT_FAILURE);
    }
    printf("\n# of CMEM blocks (doesn't include possible CMA global 'block'): %d\n", nblocks);

    if (nblocks) {
	for (i = 0; i < nblocks; i++) {
	    if (CMEM_getBlockAttrs(i, &attrs) == -1) {
		fprintf(stderr, "Failed to retrieve CMEM memory block %d bounds\n", i);
	    }
	    else {
		printf("CMEM memory block %d: phys start = %#llx, size = %#llx\n",
		       i, (unsigned long long)attrs.phys_base, attrs.size);
	    }

	    testHeap(size, i);
	    testHeap(size, i);
	    testPools(size, i);
	    testPools(size, i);
	    testCache(size, i);
	}
    }
    else {
	printf("    no physical block found, not performing block-based testing\n");
    }

    /* block 'nblocks' is the special CMEM CMA "block" */
    testPools(size, CMEM_CMABLOCKID);

    printf("\nexiting...\n");
    if (CMEM_exit() < 0) {
        fprintf(stderr, "Failed to finalize the CMEM module\n");
    }
    printf("...test done\n");

    exit(EXIT_SUCCESS);
}

