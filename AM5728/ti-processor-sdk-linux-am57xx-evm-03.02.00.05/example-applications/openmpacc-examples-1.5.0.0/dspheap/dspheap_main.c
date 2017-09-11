/******************************************************************************
 * Copyright (c) 2013-2014, Texas Instruments Incorporated - http://www.ti.com/
 *   All rights reserved.
 *
 *   Redistribution and use in source and binary forms, with or without
 *   modification, are permitted provided that the following conditions are met:
 *       * Redistributions of source code must retain the above copyright
 *         notice, this list of conditions and the following disclaimer.
 *       * Redistributions in binary form must reproduce the above copyright
 *         notice, this list of conditions and the following disclaimer in the
 *         documentation and/or other materials provided with the distribution.
 *       * Neither the name of Texas Instruments Incorporated nor the
 *         names of its contributors may be used to endorse or promote products
 *         derived from this software without specific prior written permission.
 *
 *   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *   AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *   IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *   ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 *   LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *   CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *   SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *   INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *   CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *   ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 *   THE POSSIBILITY OF SUCH DAMAGE.
 *****************************************************************************/

/*-----------------------------------------------------------------------------
* This example demonstrates how a heap may be created and used on the DSP 
* within an OpenMP target region. There are dsp builtin functions to create and 
* manipulate user defined heaps in msmc, ddr and local memory regions. These 
* heaps are persistent as long as the underlying memory for them is allocated.
* In this example we create buffers from contiguous shared memory regions that 
* provide for the underlying memory store.  The heaps are active and persistent
* from the time they are initialized until the buffers are deallocated.
*
* Additionally, the standard malloc, calloc, free, etc calls are already 
* supported on the dsp, but the underlying memory for that heap is limited.
* It currently is approximately 8MB.  If your heap needs are under that size, 
* and DDR allocation is ok for you, then the below mechanism is not needed.
*----------------------------------------------------------------------------*/
#include <ti_omp_device.h>

extern void heap_init_ddr(char* p, size_t bytes); 
extern void heap_init_msmc(char *p, size_t bytes); 
extern void alloc_and_free(size_t bytes);
extern void l2_alloc_and_free(size_t bytes);
extern void alloc_only(size_t bytes);

int main(int argc, char *argv[])
{

     /*------------------------------------------------------------------------
     * From the host, create the underlying memory store for the heaps       
     *-----------------------------------------------------------------------*/
     int ddr_heap_size  = 16 << 20; //16MB
     int msmc_heap_size = 1 << 19;  //512KB
     int l2_heap_size = 1 << 10;    //1KB

     char* HeapDDR = (char*) __malloc_ddr(ddr_heap_size);
     char* HeapMSMC = (char*) __malloc_msmc(msmc_heap_size);

     /*------------------------------------------------------------------------
     * Initialize the pre-allocated buffers as new DDR and MSMC heaps 
     * accesible to DSP cores.                                              
     *-----------------------------------------------------------------------*/
     heap_init_ddr (HeapDDR,  ddr_heap_size);
     heap_init_msmc(HeapMSMC, msmc_heap_size);

     /*------------------------------------------------------------------------
     * On each DSP core, alloc memory from both ddr and msmc and then free it.
     *-----------------------------------------------------------------------*/
     alloc_and_free(1024);

     /*------------------------------------------------------------------------
     * On each core alloc memory from both ddr and msmc. Should see same memory 
     * from above alloc_and_free call.  This time the memory is not freed.
     *-----------------------------------------------------------------------*/
     alloc_only(1024);

     /*------------------------------------------------------------------------
     * Again, alloc on each core. Since the previous call did not free, these
     * allocations should be in separate memory from the last set.
     *-----------------------------------------------------------------------*/
     alloc_only(1024);

     /*------------------------------------------------------------------------
     * On each DSP core, alloc memory from L2 SRAM and then free it.
     *-----------------------------------------------------------------------*/
     l2_alloc_and_free(l2_heap_size);

     /*------------------------------------------------------------------------
     * Free the underlying buffers used as DDR and MSMC heaps
     *-----------------------------------------------------------------------*/
    __free_ddr(HeapDDR);
    __free_msmc(HeapMSMC);

    return 0;
}
