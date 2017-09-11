/******************************************************************************
 * Copyright (c) 2014, Texas Instruments Incorporated - http://www.ti.com/
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
#pragma omp declare target
#include <stdlib.h>
#include <stdio.h>
#include <omp.h>
#include "ti_omp_device.h"
#pragma omp end declare target

/*-----------------------------------------------------------------------------
* User-controlled DSP heaps are initialized within a target region. The call 
* to __heap_init_xxx can be included within any target region. However the 
* initialization function must be called before any __malloc_xxx calls are 
* made.
*
* User-controlled DSP DDR and MSMC heaps can be persistent across target 
* regions as long as the underlying memory (aka buffers pointed to by p are 
* not deallocated.
*----------------------------------------------------------------------------*/
void heap_init_ddr(char* p, size_t bytes) 
{ 
#pragma omp target map(to:bytes,p[0:bytes])
   {
      __heap_init_ddr(p,bytes); 
   }
}

void heap_init_msmc(char *p, size_t bytes) 
{ 
#pragma omp target map(to: bytes, p[0:bytes])
   {
      __heap_init_msmc(p,bytes); 
   }
}

/*-----------------------------------------------------------------------------
* The DSP core executing the enclosed target region will allocate from the 
* DDR and MSMC heaps and then free the memory.
*----------------------------------------------------------------------------*/
void alloc_and_free(size_t bytes)
{
#pragma omp target map(to: bytes) 
   {
      char *p1 = (char *) __malloc_ddr(bytes);
      char *p2 = (char *) __malloc_msmc(bytes);

      if (!p1 || !p2) 
         printf("FAIL!\n");
      else
      {
         printf("DDR  heap pointer is 0x%08x\n", p1);
         printf("MSMC heap pointer is 0x%08x\n", p2);
      }

      __free_ddr(p1);
      __free_msmc(p2);
   }
}

/*-----------------------------------------------------------------------------
* User-controlled DSP heaps in L2 SRAM are local to the core that initialized 
* the heap. Heaps initialized in L2 SRAM do not persist across target regions 
* or kernels.
* In the enclosed target region, p will not be copied to DSPs' L2 SRAM because 
* it is mapped with the "local" map type. Instead p gets allocated in each 
* DSP's local memory at the start of the target region.
*----------------------------------------------------------------------------*/
void l2_alloc_and_free(size_t bytes)
{
   char *p = NULL;
#pragma omp target map(to:bytes) map(local:p[0:bytes])
   {
      char *p1;
      __heap_init_l2(p, bytes); 
      p1 = (char *) __malloc_l2(bytes);
      if (!p1) 
         printf("FAIL!\n");
      else
         printf("L2SRAM heap pointer is 0x%08x\n", p1);
   }
}

/*-----------------------------------------------------------------------------
* The DSP core executing the enclosed target region will allocate from the 
* DDR and MSMC heaps and the memory is not freed. The active pointers p1 and 
* p2 could be returned to the host application, via a map clause on the target 
* construct. They could then subsequently be passed to other target regions.  
* However the values should not be dereferenced on the host, because the DSP 
* addresses are not valid linux system addresses.
*----------------------------------------------------------------------------*/
void alloc_only(size_t bytes)
{
#pragma omp target map(to: bytes)
   {
      char *p1 = __malloc_ddr(bytes);
      char *p2 = __malloc_msmc(bytes);

      if (!p1 || !p2) 
         printf("FAIL!\n");
      else
      {
         printf("DDR  heap pointer is 0x%08x\n", p1);
         printf("MSMC heap pointer is 0x%08x\n", p2);
      }
   }
}
