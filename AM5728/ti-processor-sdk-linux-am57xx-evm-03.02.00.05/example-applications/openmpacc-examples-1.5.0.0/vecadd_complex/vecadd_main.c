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
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "vecadd.h"

#define RAND_MAX_A 64
#define RAND_MAX_B 256
#define EPISILON  0.00001

/* ======================================================================== */
/*  MAIN                                                                    */
/* ======================================================================== */
int main()
{
    int          num_errors = 0;
    const int    print_nerrors = 12;
    const int    NumElements     = 8*1024;

    complex_t  srcA  [NumElements];
    complex_t  srcB  [NumElements];
    complex_t  dst   [NumElements];
    complex_t  Golden[NumElements];
    
    /* ---------------------------------------------------------------- */
    /*  Initialized input arrays with random test data.                 */
    /* ---------------------------------------------------------------- */
    /* Initialize Random Number Seed */
    srand(time(NULL));
    
    
    for (int i=0; i < NumElements; i++) 
    {   
        real_t valA = (rand() % RAND_MAX_A + 1) * 1.0;
        real_t valB = (rand() % RAND_MAX_B + 1) * 1.0; 
        srcA[i] = valA + valA * I;
        srcB[i] = valB + valB * I;
        dst[i] = 0.0 + 0.0 * I;
        Golden[i]   =   srcA[i] + srcB[i];
    }   

    /* ---------------------------------------------------------------- */
    /* Call vadd_openmp target code                                     */
    /* ---------------------------------------------------------------- */
    vadd_openmp(srcA,srcB,dst,NumElements);
    
    /* ---------------------------------------------------------------- */
    /* Perform error checking                                           */
    /* ---------------------------------------------------------------- */
    for (int i=0; i < NumElements; i++)
    {
        real_t diff = cabsf(Golden[i]) - cabsf(dst[i]);

        if (diff < -EPISILON ||  diff > EPISILON) 
        { 
            if((num_errors += 1) < print_nerrors)
                printf("Error %d: %f <==> %f\n", i, cabsf(Golden[i]), 
                      cabsf(dst[i]));
        }
    }
    if (num_errors > 0)  printf( "FAIL with %d errors!\n", num_errors );
    else                 printf( "PASS \n"); 

    return 0;
}
