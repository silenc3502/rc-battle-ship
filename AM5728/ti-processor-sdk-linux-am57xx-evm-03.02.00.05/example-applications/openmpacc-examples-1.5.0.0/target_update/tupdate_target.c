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
#include <time.h>
#define RAND_MAX_A 64
#define RAND_MAX_B 256
#pragma omp end declare target

void vadd_target_update(float *a, float *b, float *c, float* Golden, int size)
{
    /* Create device buffers for a, b, c */
    #pragma omp target data map(to:a[0:size], b[0:size], c[0:size])
    {
        /* ---------------------------------------------------------------- */
        /*  Initialized input arrays with random test data.                 */
        /* ---------------------------------------------------------------- */
        /* Initialize Random Number Seed */
        srand(time(NULL));

        int i; 
        for (i=0; i < size; ++i) 
        {
            a[i] = (rand() % RAND_MAX_A + 1) * 1.0; 
            b[i] = (rand() % RAND_MAX_B + 1) * 1.0;
            c[i] = 0;
            Golden[i]   =   a[i] + b[i];
        }   
       
        /* Update device buffers for a,b,c 
         * Host -> Device transfers happen here */
        #pragma omp target update to(a[0:size], b[0:size], c[0:size])

        #pragma omp target map(to:a[0:size], b[0:size], c[0:size])
        {
            int i;
            #pragma omp parallel for
            for (i = 0; i < size; i++)
                c[i] += a[i] + b[i];
        }
        
        /* Update the host buffer for c 
         * Device -> Host transfer happens here */
        #pragma omp target update from(c[0:size])
    }
}
