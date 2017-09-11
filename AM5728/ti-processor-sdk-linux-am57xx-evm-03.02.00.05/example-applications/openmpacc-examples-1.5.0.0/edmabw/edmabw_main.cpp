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
#include <iostream>
#include <cstdio>
#include "ti_omp_device.h"

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

extern "C"
{
    bandwidth_t TestBandwidthOnCore(char *ddr, char *msmc, int size, int core_num);
    /* Host stubs */
    float bandwidth(char* src,char* ddr, int size){ return 0;}
}

int main(int argc, char *argv[])
{
#if 0
    int size = 512 << 10; // 512K
#else
    int size = 128 << 10; // 128K
#endif
    int device_id = 0;
    bandwidth_t results[8];
    char* ddr_buf   = (char*)__malloc_ddr(size);
    char* msmc_buf  = (char*)__malloc_msmc(size);

    for (int i = 0; i < 8; i++)
    {
        results[i] =  TestBandwidthOnCore(ddr_buf, msmc_buf, size, i);
    }
  
    printf("Single channel EDMA bandwidth measured in GB/s\n");
    printf("==============\n");
    printf("From DSP Core:\t");
    for (int i = 0; i < 8; i++) printf("%d\t", i);
    printf("\n");
    printf("==============\n");
    
    printf("ddr  => ddr  :\t");
    for (int i = 0; i < 8; i++) printf("%.3f\t", results[i].dd);
    printf("\n");
    
    printf("ddr  => msmc :\t");
    for (int i = 0; i < 8; i++) printf("%.3f\t", results[i].dm);
    printf("\n");

    printf("ddr  => l2   :\t");
    for (int i = 0; i < 8; i++) printf("%.3f\t", results[i].dl);
    printf("\n");

    printf("msmc => ddr  :\t");
    for (int i = 0; i < 8; i++) printf("%.3f\t", results[i].md);
    printf("\n");

    printf("msmc => msmc :\t");
    for (int i = 0; i < 8; i++) printf("%.3f\t", results[i].mm);
    printf("\n");

    printf("msmc => l2   :\t");
    for (int i = 0; i < 8; i++) printf("%.3f\t", results[i].ml);
    printf("\n");

    printf("l2   => ddr  :\t");
    for (int i = 0; i < 8; i++) printf("%.3f\t", results[i].ld);
    printf("\n");

    printf("l2   => msmc :\t");
    for (int i = 0; i < 8; i++) printf("%.3f\t", results[i].lm);
    printf("\n");

    printf("l2   => l2   :\t");
    for (int i = 0; i < 8; i++) printf("%.3f\t", results[i].ll);
    printf("\n");

    __free_ddr(ddr_buf);
    __free_msmc(msmc_buf);
}
