/******************************************************************************
 * Copyright (c) 2013-2015, Texas Instruments Incorporated - http://www.ti.com/
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
/******************************************************************************
* FILE: matmpy/main.c 
* Purpose: to demonstrate how to use DGEMM for matrix multiplication. 
*
* DGEMM  performs one of the matrix-matrix operations
*
*    C := alpha*op( A )*op( B ) + beta*C,
*
*    where  op( X ) is one of
*
*    op( X ) = X   or   op( X ) = X**T,
*
* alpha and beta are scalars, and A, B and C are matrices, with op(A)
* an m by k matrix,  op(B) a k by n matrix and C an m by n matrix.
******************************************************************************/
 
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <CL/cl_ext.h>

#include "cblas.h"

/* Number of elements in matrix to display */

/* For profiling */
#define tick()  clock_gettime(CLOCK_MONOTONIC, &t0);
#define tock() (clock_gettime(CLOCK_MONOTONIC, &t1), \
                        t1.tv_sec - t0.tv_sec + (t1.tv_nsec - t0.tv_nsec) / 1e9)
#define fout stdout

double *A, *B, *C;
int m, n, k;
double alpha, beta;
struct timespec t0, t1;
double secs = 0.0;

static void report_flops(double secs, int m, int n, int k, int N)
{
  fprintf(fout,"Total time for %d tests: %8.6fs, %5.3f Gflops\n",
          N, secs, (float)N*2*m*n*k / (secs * 1e9));
}

double matrix_mult(void) {
    int i,j;
    for (i = 0; i < (m*k); i++) {
        A[i] = (double)rand()/RAND_MAX;
    }

    for (i = 0; i < (k*n); i++) {
        B[i] = (double)rand()/RAND_MAX;
    }

    for (i = 0; i < (m*n); i++) {
        C[i] = 0.0;
    }

    tick();
    cblas_dgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans, m, n, k, alpha, A, k, B, n, beta, C, n);
    secs += tock();

    /* We do a simplistic checksum across a subset of the result matrix */
    double checksum = 0.0;
    for (i=0; i<m; i++) 
      for (j=0; j<n; j++) 
        checksum += C[j+i*n];
    return checksum;
}

int main()
{
    int t;
    double checksum;
    char *ti_cblas_offload_env;
    int numtests = 10;

    /* configuration */
    m = k = n = 1000;
    alpha = 0.7; 
    beta  = 1.3; 

    /* allocate the matrices */
    A = (double *)__malloc_ddr( m*k*sizeof( double ) );
    B = (double *)__malloc_ddr( k*n*sizeof( double ) );
    C = (double *)__malloc_ddr( m*n*sizeof( double ) );
    if (A == NULL || B == NULL || C == NULL) {
      printf( "\nERROR: Can't allocate memory for matrices. Aborting... \n\n");
      __free_ddr(A);
      __free_ddr(B);
      __free_ddr(C);
      return 1;
    }
    
    srand(123456789);

    /* Check the environment variable that controls offloading */
    ti_cblas_offload_env = getenv("TI_CBLAS_OFFLOAD");
    if(ti_cblas_offload_env == NULL) {
      printf("Environment variable TI_CBLAS_OFFLOAD is not defined.");
      printf("Use default offloading configuration:\n");
      printf("\tBLAS level 1: running on ARM.\n");
      printf("\tBLAS level 2: running on ARM.\n");
      printf("\tBLAS level 3: running on ARM or DSP based on matrix sizes.\n");
    }
    else {
      printf("TI_CBLAS_OFFLOAD is defined as %s\n", ti_cblas_offload_env);
    }

    printf ("A(%ix%i) X B(%ix%i) => C(%ix%i)\n", m, k, k, n, m, n);

    printf("Warming caches (by doing a single matrix-multiply)..\n");
    checksum = matrix_mult();

    /* reset secs, so we can now begin the real timing */
    secs = 0;

    printf("Now doing %d tests after warming caches\n", numtests);
    for (t=0; t<numtests; t++) {
      checksum += matrix_mult();
    }
    report_flops(secs, m, n, k, numtests);

    printf("Passed.\n");

    __free_ddr(A);
    __free_ddr(B);
    __free_ddr(C);

    return 0;
}


