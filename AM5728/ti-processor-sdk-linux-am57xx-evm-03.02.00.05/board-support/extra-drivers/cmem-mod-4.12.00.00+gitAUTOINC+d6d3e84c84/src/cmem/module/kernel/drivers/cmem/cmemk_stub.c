/*
 *  Copyright (C) 2013 Texas Instruments Incorporated - http://www.ti.com
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; version 2 of the License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>
 */
/*
 * cmemk_stub.c
 *
 * This file provides support for the CMEM CMA-based allocations by the
 * loadable kernel module cmemk.ko.  It reserves CMA regions (areas) for
 * later allocations by CMEM.  These CMA regions are defined according to
 * two kernel command line parameters:
 *     cmem_cma_pools=<n>x<s>[,<n>x<s>,...]
 *     cmem_cma_heapsize=<sKMG>
 * where
 *     <n>x<s> specifies a pool of <n> buffers of size <s> bytes
 *     <sKMG> is a heapsize specified according to 's' KBs, MBs, or GBs (or
 *          just 's' bytes when no letter suffix is specified)
 *
 * This file must be builtin to the kernel, since it contains a function
 * that must be called extremely early in the boot procedure.
 *
 * It is suggested that this file be placed according to the directory
 * structure as placed in the linuxutils product, in
 *     <kernel>/drivers/cmem/cmemk_stub.c
 *     <kernel>/drivers/cmem/Makefile
 *     <kernel>/include/linux/cmemk_stub.h - more than a suggestion to place
 *         this file here, as the loadable portion of CMEM, cmemk.ko, expects
 *         it here
 * In doing so, the contents of
 *     <linuxutils>/drivers/Makefile
 * should be copied to the existing
 *     <kernel>/drivers/Makefile
 *
 * To give "energy" to this stub, you must place a call to
 *     cmem_reserve_cma();
 * in your machine's "early boot" '.reserve' procedure.  This is accomplished
 * by either assigning MACHINE_START.reserve to cmem_reserve_cma, or, if your
 * machine already has a .reserve function assignment, placing a call to
 * cmem_reserve_cma() somewhere in that function.  For example, for the
 * OMAP-L138 "Hawkboard", the following code in
 *     arch/arm/mach-davinci/board-omapl138-hawk.c
 * illustrates this:
 * MACHINE_START(OMAPL138_HAWKBOARD, "AM18x/OMAP-L138 Hawkboard")
 *        .atag_offset    = 0x100,
 *        .map_io         = omapl138_hawk_map_io,
 *        .init_irq       = cp_intc_init,
 *        .timer          = &davinci_timer,
 *        .init_machine   = omapl138_hawk_init,
 *        .init_late      = davinci_init_late,
 *        .dma_zone_size  = SZ_128M,
 *        .restart        = da8xx_restart,
 *        .reserve        = cmem_reserve_cma,
 * MACHINE_END
 */
#include <linux/device.h>
#include <linux/dma-mapping.h>
#include <linux/dma-contiguous.h>
#include <linux/export.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/printk.h>
#include <linux/string.h>

#include <linux/cmemk_stub.h>

int cmem_cma_npools = 0;
int cmem_cma_heapsize = 0;
struct device cmem_cma_dev[CMEM_MAXPOOLS];
struct pool_object cmem_cma_p_objs[CMEM_MAXPOOLS];

EXPORT_SYMBOL(cmem_cma_npools);
EXPORT_SYMBOL(cmem_cma_heapsize);
EXPORT_SYMBOL(cmem_cma_dev);
EXPORT_SYMBOL(cmem_cma_p_objs);

static int __init early_cmemheapsize(char *p)
{
    cmem_cma_heapsize = memparse(p, NULL);

    pr_debug("cmemheapsize: heapsize=0x%x\n", cmem_cma_heapsize);

    return 0;
}
early_param("cmem_cma_heapsize", early_cmemheapsize);

static int __init early_cmempools(char *p)
{
    int done = 0;
    int n;
    char *x;

    do {
	x = strchr(p, 'x');
	if (x != NULL) {
	    *x = '\0';
	    n = cmem_cma_npools;
	    if (n == CMEM_MAXPOOLS) {
		pr_err("cmempools: CMEM_MAXPOOLS reached!\n");

		break;
	    }
	    cmem_cma_p_objs[n].numbufs = memparse(p, NULL);
	    cmem_cma_p_objs[n].reqsize = memparse(x + 1, &x);

	    pr_debug("cmempools: pool %d = %d * 0x%x\n", n,
	             cmem_cma_p_objs[n].numbufs, cmem_cma_p_objs[n].reqsize);

	    cmem_cma_npools++;

	    if (*x++ == ',') {
		p = x;
	    }
	    else {
		done = 1;
	    }
	}
	else {
	    done = 1;
	}
    } while (!done);

    return 0;
}
early_param("cmem_cma_pools", early_cmempools);

static void cmem_device_release(struct device *dev)
{
    pr_debug("cmem_device_release(%p)...\n", dev);

    put_device(dev);
}


/* this function is registered in arch/arm/plat-omap/common.c */
void __init cmem_reserve_cma(void)
{
    int poolsize;
    int reqsize;
    int size;
    int nbufs;
    int i;
    int rv;

    pr_debug("cmem_reserve_cma()...\n");

    if (cmem_cma_heapsize) {
	/* heap "pool" */
	pr_debug("cmem_reserve_cma: heapsize 0x%x specified\n",
	         cmem_cma_heapsize);
	cmem_cma_p_objs[cmem_cma_npools].reqsize = cmem_cma_heapsize;
	cmem_cma_p_objs[cmem_cma_npools].numbufs = 1;

	cmem_cma_npools++;
    }

    for (i = 0; i < cmem_cma_npools; i++) {
	reqsize = cmem_cma_p_objs[i].reqsize;
	if (reqsize != 0) {
	    nbufs = cmem_cma_p_objs[i].numbufs;
	    size = round_up(reqsize, PAGE_SIZE);
	    poolsize = nbufs * size;
	    rv = dma_declare_contiguous(&cmem_cma_dev[i], poolsize, 0,
	                                0xffffffff);

	    if (rv) {
		pr_err("cmem_reserve_cma: dma_declare_contiguous failed %d\n",
		       rv);

		/* size of 0 means no buffers available */
		cmem_cma_p_objs[i].size = 0;
	    }
	    else {
		pr_debug("cmem_reserve_cma: dma_declare_contiguous succeeded\n");
		cmem_cma_dev[i].release = cmem_device_release;

		/* numbufs and reqsize already set in early_cmempools() */
		cmem_cma_p_objs[i].size = size;
	    }
	}
    }
}

