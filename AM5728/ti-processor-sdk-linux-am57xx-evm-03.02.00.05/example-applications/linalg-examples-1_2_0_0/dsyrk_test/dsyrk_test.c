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
* FILE: dsyrk_test.c 
* Purpose: to test BLAS function DSYRK.
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

#define START_SIZE_RECTAN_MATRIX 128
#define NUM_MATRIX_SIZE_TO_TEST  4
#define NUM_TEST_RUN 5

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
enum CBLAS_TRANSPOSE transA = CblasNoTrans;
enum CBLAS_UPLO      uplo   = CblasUpper;

extern int TI_CBLAS_L3_OFFLOAD;
/*-----------------------------------------------------------------------------
* Prototypes
*----------------------------------------------------------------------------*/
int check_results(const double *C1, const double *C2, int N, int K);
int run_dsyrk(int N, int K, float *time, float *gflops);

/*-----------------------------------------------------------------------------
* MAIN
*----------------------------------------------------------------------------*/
int main()
{
    int dsyrk_err;
    int N, K, n, k;
    float time_secs, gflops;
    FILE *fp_time;  
    FILE *fp_gflops;  
  
    fp_time = fopen("dsyrk_time.dat","w");
    fp_gflops = fopen("dsyrk_gflops.dat","w");
    
    srand(12345);
    
    /* setting up TI CBLAS during first call */
    run_dsyrk(1000, 1000, &time_secs, &gflops);
    
    /* sweep K, and N */    
    for (N=START_SIZE_RECTAN_MATRIX,n=0; n<NUM_MATRIX_SIZE_TO_TEST; n++,N*=2) 
    {
        for (K=START_SIZE_RECTAN_MATRIX,k=0; k<NUM_MATRIX_SIZE_TO_TEST; k++,K*=2) 
        {
            printf("Running DSYRK for (N,K)=(%d,%d)\n", N,K);
                
            dsyrk_err = run_dsyrk(N, K, &time_secs, &gflops);
          
            if(dsyrk_err == -1) {  /* out of memory for DSP offloading */
                printf("Out of memory for (N,K) = (%d,%d).\n", N,K);
                fprintf(fp_time, "%6d\t%6d\t%10.8e\n", N, K, -1.0);
                fprintf(fp_gflops, "%6d\t%6d\t%10.8e\n", N, K, -1.0);
            }
            else {
                fprintf(fp_time, "%6d\t%6d\t%10.8e\n", N, K, time_secs);
                fprintf(fp_gflops, "%6d\t%6d\t%10.8e\n", N, K, gflops);
            }
        }
    }        
    fclose(fp_time);
    fclose(fp_gflops);

    printf("Passed.\n");
    
    return 0;
}


int run_dsyrk(int N, int K, float *time, float *gflops)
{
    long long i, size_A, size_C;
    int    iter;
    float  time_secs, total_time, total_gflops, gflops_iter;
    float  operation_count = 1.0*(double)N*(double)N*(double)K;
    int    err_code = 0;
    
    total_time= 0.0;
	total_gflops = 0.0;
    size_A = (long long)N*(long long)K;
    size_C = (long long)N*(long long)N;
    
    if(  (size_A*sizeof(double)>(long long)0x0ffffffff) 
       ||(size_C*sizeof(double)>(long long)0x0ffffffff) ) {
        return (-1);
    }
    
    for (iter = 0; iter < NUM_TEST_RUN; iter++)
    {      
      /*-------------------------------------------------------------------------
      * Allocate space for the matrices.  The matrices that will be passed to 
      * the DSP are allocated using device memory.  
      *------------------------------------------------------------------------*/
      double *A = (double *) __malloc_ddr(size_A*sizeof(double));
      double *C = (double *) __malloc_ddr(size_C*sizeof(double));
  
      if (!A || !C)
      {
          printf("Could not allocate enough space for the arrays!");
          if(A) __free_ddr(A);
          if(C) __free_ddr(C);
         
          return (-1);
      }
  
      /*-------------------------------------------------------------------------
      * Initialize matrices 
      *------------------------------------------------------------------------*/
      for (i = 0; i < (long long)N*K; ++i) A[i] = (double)rand()/RAND_MAX;
      for (i = 0; i < (long long)N*N; ++i) C[i] = (double)rand()/RAND_MAX;
  
      int lda = ((order == CblasColMajor && transA == CblasNoTrans) ||
              (order == CblasRowMajor && transA == CblasTrans)) ? N : K;
  
      int ldc = N;
  
      fflush(stdout);
      
      /*------------------------------------------------------------------------
      * Time dsyrk
      *-----------------------------------------------------------------------*/
      tick();
      cblas_dsyrk(order,uplo,transA,N,K,alpha,A,lda,beta,C,ldc);
      time_secs = tock();
      total_time += time_secs;
      gflops_iter = operation_count/time_secs*1e-9;
      total_gflops += gflops_iter;
  
      __free_ddr(A);
      __free_ddr(C);
    }
    
    *gflops = total_gflops / (double)NUM_TEST_RUN;
    *time   = total_time / (double)NUM_TEST_RUN;
    
    return err_code;
}

