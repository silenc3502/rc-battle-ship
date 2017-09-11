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

#ifndef _CMEMK_STUB_H
#define _CMEMK_STUB_H

#if defined (__cplusplus)
extern "C" {
#endif

#include <linux/device.h>

#define CMEM_MAXPOOLS 8

struct pool_object {
    struct list_head freelist;
    struct list_head busylist;
    unsigned int numbufs;
    unsigned int size;
    unsigned int reqsize;
};

extern struct pool_object cmem_cma_p_objs[];
extern struct device cmem_cma_dev[];
extern int cmem_cma_heapsize;
extern int cmem_cma_npools;

#if defined (__cplusplus)
}
#endif

#endif
