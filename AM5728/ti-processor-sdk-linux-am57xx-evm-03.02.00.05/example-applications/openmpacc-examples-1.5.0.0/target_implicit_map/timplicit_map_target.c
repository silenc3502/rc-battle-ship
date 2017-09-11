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
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#define RAND_MAX_A      64
#define RAND_MAX_B      256
#define EPISILON        0.00001
#define NUM_ELEMENTS    8*1024
#pragma omp end declare target

int vadd_implicit_map()
{
    int i;
    int print_nerrors = 12;
    int num_errors    = 0;
    float srcA[NUM_ELEMENTS];
    float srcB[NUM_ELEMENTS];
    float dst[NUM_ELEMENTS];
    float Golden[NUM_ELEMENTS];
   
    /* ---------------------------------------------------------------- */
    /* Offload random number generation to DSPs with                    */
    /* an implicit mapping of arrays                                    */
    /* ---------------------------------------------------------------- */
    #pragma omp target
    {
        /* Initialize Random Number Seed */
        srand(time(NULL));
        #pragma omp parallel for private(i)
        for (i=0; i < NUM_ELEMENTS; ++i) 
        {
            srcA[i] = (rand() % RAND_MAX_A + 1) * 1.0; 
            srcB[i] = (rand() % RAND_MAX_B + 1) * 1.0;
            dst[i] = 0;
            Golden[i]   =  srcA[i] + srcB[i];
        }   
    }
    /* ---------------------------------------------------------------- */
    /* Offload vector addition to DSPs with                             */
    /* an implicit mapping of arrays                                    */
    /* ---------------------------------------------------------------- */
    #pragma omp target
    #pragma omp parallel for private(i)
    for (i = 0; i < NUM_ELEMENTS; i++)
        dst[i] += srcA[i] + srcB[i];

    /* ---------------------------------------------------------------- */
    /* Perform error checking                                           */
    /* ---------------------------------------------------------------- */
    for (i=0; i < NUM_ELEMENTS; ++i)
    {
        if (Golden[i] - dst[i] < -EPISILON || Golden[i] - dst[i] > EPISILON) 
        { 
            if((num_errors += 1) < print_nerrors)
                printf("Error %d: %f <==> %f\n", i, Golden[i], dst[i]);
        }
    }
    return num_errors;

}
