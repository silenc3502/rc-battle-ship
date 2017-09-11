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
#pragma omp declare target
#include <stdio.h>
#include <stdint.h>
#include <omp.h>
uint32_t __core_num(void);
float bandwidth(char*,char*,int);
typedef struct 
{
    float dd;
    float dm;
    float dl;
    float md;
    float mm;
    float ml;
    float ld;
    float lm;
    float ll;
} bandwidth_t;
#pragma omp end declare target

bandwidth_t TestBandwidthOnCore(char *ddr, char *msmc, int size, int core_num)
{
    char* l2 = NULL;
    bandwidth_t results;

    #pragma omp target map(to:ddr[0:size],msmc[0:size], core_num) \
                       map(local:l2[0:size]) \
                       map(from: results)
    {
        #pragma omp parallel default(none) shared(ddr,msmc,l2,core_num) 
        {
            if (__core_num() == core_num)
            {
                int trials = 100;
                int i;

                float dd = 0;
                float dm = 0;
                float dl = 0;
                float md = 0;
                float mm = 0;
                float ml = 0;
                float ld = 0;
                float lm = 0;
                float ll = 0;

                for(i=0;i<trials;++i) dd += bandwidth((char*)ddr,(char*)ddr,size);
                for(i=0;i<trials;++i) dm += bandwidth((char*)ddr,(char*)msmc,size);
                for(i=0;i<trials;++i) dl += bandwidth((char*)ddr,(char*)l2,size);
                for(i=0;i<trials;++i) md += bandwidth((char*)msmc,(char*)ddr,size);
                for(i=0;i<trials;++i) mm += bandwidth((char*)msmc,(char*)msmc,size);
                for(i=0;i<trials;++i) ml += bandwidth((char*)msmc,(char*)l2,size);
                for(i=0;i<trials;++i) ld += bandwidth((char*)l2,(char*)ddr,size);
                for(i=0;i<trials;++i) lm += bandwidth((char*)l2,(char*)msmc,size);
                for(i=0;i<trials;++i) ll += bandwidth((char*)l2,(char*)l2,size);
                
                results.dd = dd/trials;
                results.dm = dm/trials;
                results.dl = dl/trials;
                results.md = md/trials;
                results.mm = mm/trials;
                results.ml = ml/trials;
                results.ld = ld/trials;
                results.lm = lm/trials;
                results.ll = ll/trials;
            }
        }
    }

    return results;
}
