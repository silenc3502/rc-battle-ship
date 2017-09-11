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
* FILE: dgemm_test.c 
* Purpose: to test BLAS function DGEMM.
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
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <time.h>
#include <CL/cl_ext.h>

#include "cblas.h"
#ifdef __cplusplus
extern "C" {
#endif
#include "cblas.h"
#ifdef __cplusplus
}
#endif

#define TUNING_START_SIZE_RECTAN_MATRIX 128
#define NUM_MATRIX_SIZE_TO_BENCHMARK 4
#define HAS_MEMORY   1
#define NO_MEMORY    0
#define OFFLOAD      1
#define NO_OFFLOAD   0

#define NUM_TEST_RUN 1


/*-----------------------------------------------------------------------------
* Timing Setup
*----------------------------------------------------------------------------*/
struct timespec t0,t1;
#define tick()  clock_gettime(CLOCK_MONOTONIC, &t0);
#define tock() (clock_gettime(CLOCK_MONOTONIC, &t1), \
                        t1.tv_sec - t0.tv_sec + (t1.tv_nsec - t0.tv_nsec) / 1e9)

/*-----------------------------------------------------------------------------
* Global Variables
*----------------------------------------------------------------------------*/
double alpha           = 0.7; 
double beta            = 0.3;
enum CBLAS_ORDER     order  = CblasColMajor; 
//enum CBLAS_ORDER     order  = CblasRowMajor;
enum CBLAS_TRANSPOSE transA = CblasNoTrans;
enum CBLAS_TRANSPOSE transB = CblasNoTrans;

extern int TI_CBLAS_L3_OFFLOAD;
/*-----------------------------------------------------------------------------
* Prototypes
*----------------------------------------------------------------------------*/
int run_dgemm(int M, int N, int K, float *time, float *gflops);

/*-----------------------------------------------------------------------------
* MAIN
*----------------------------------------------------------------------------*/
int main()
{
    int num_size, dgemm_err;
    int M, N, K, m, n, k;
    float time_secs, gflops;
    FILE *fp_time, *fp_gflops;  
  
    fp_time = fopen("dgemm_time.dat","w");
    fp_gflops = fopen("dgemm_gflops.dat","w");

    srand(12345);
    
	/* setting up TI CBLAS during first call */
	run_dgemm(1000, 1000, 1000, &time_secs, &gflops);
	
    /* sweep M, K, and N */    
    for (M=TUNING_START_SIZE_RECTAN_MATRIX,m=0; m<NUM_MATRIX_SIZE_TO_BENCHMARK; m++,M*=2) 
    {
        for (N=TUNING_START_SIZE_RECTAN_MATRIX,n=0; n<NUM_MATRIX_SIZE_TO_BENCHMARK; n++,N*=2) 
        {
            for (K=TUNING_START_SIZE_RECTAN_MATRIX,k=0; k<NUM_MATRIX_SIZE_TO_BENCHMARK; k++,K*=2) 
            {
                printf("Running DGEMM for (M,N,K) = (%d,%d,%d).\n", M,N,K);
				
                dgemm_err = run_dgemm(M, N, K, &time_secs, &gflops);
          
                if(dgemm_err == -1) {  /* out of memory for DSP offloading */
                    printf("Out of memory for (M,N,K) = (%d,%d,%d).\n", M,N,K);
                }
                else {
                    fprintf(fp_time,  "%6d\t%6d\t%6d\t%10.8e\n", M, N, K, time_secs);
                    fprintf(fp_gflops, "%6d\t%6d\t%6d\t%10.8e\n", M, N, K, gflops);
                }
            }
        }
    }
        
    fclose(fp_time);
    fclose(fp_gflops);
    
	printf("Passed.\n");
    return 0;
}


int run_dgemm(int M, int N, int K, float *time, float *gflops)
{
    int    iter;
    long long i;
    double time_secs, total_time;
    double operation_count = 2.0*(double)M*(double)N*(double)K;
    double total_GFLOPS = 0.0f;
    int    err_code = 0;
    
    total_time = 0.0;
    for (iter = 0; iter < NUM_TEST_RUN; iter++)
    {      
      /*-------------------------------------------------------------------------
      * Allocate space for the matrices.  
      *------------------------------------------------------------------------*/
      double *A = (double *) __malloc_ddr((long long)M*(long long)K*(long long)sizeof(double));
      double *B = (double *) __malloc_ddr((long long)K*(long long)N*(long long)sizeof(double));
      double *C = (double *) __malloc_ddr((long long)M*(long long)N*(long long)sizeof(double));
  
      if (!A || !B || !C)
      {
          printf("Could not allocate enough space for the arrays!");
          if(A) __free_ddr(A);
          if(B) __free_ddr(B);
          if(C) __free_ddr(C);
          
          return (-1);
      }
  
      /*-------------------------------------------------------------------------
      * Initialize matrices 
      *------------------------------------------------------------------------*/
      for (i = 0; i < (long long)M*K; ++i) A[i] = (double)rand()/RAND_MAX;// (double)(rand() % 5 + 1);
      for (i = 0; i < (long long)K*N; ++i) B[i] = (double)rand()/RAND_MAX;// (double)(rand() % 5 + 1);
      for (i = 0; i < (long long)M*N; ++i) C[i] = 0;
  
      int lda = ((order == CblasColMajor && transA == CblasNoTrans) ||
              (order == CblasRowMajor && transA == CblasTrans)) ? M : K;
  
      int ldb = ((order == CblasColMajor && transB == CblasNoTrans) ||
              (order == CblasRowMajor && transB == CblasTrans)) ? K : N;
  
      int ldc = (order == CblasColMajor) ? M : N;
  
      fflush(stdout);
  
      /*------------------------------------------------------------------------
      * Run and time dgemm
      *-----------------------------------------------------------------------*/
      tick();
      cblas_dgemm(order,transA,transB,M,N,K,alpha,A,lda,B,ldb,beta,C,ldc);
      time_secs = tock();
      total_time += time_secs;
      total_GFLOPS += operation_count/time_secs*1e-9;
  
      __free_ddr(A);
      __free_ddr(B);
      __free_ddr(C);
    }
    
    *gflops = total_GFLOPS / (double)NUM_TEST_RUN;
    *time   = total_time / (double)NUM_TEST_RUN;
    
    return err_code;
}
