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
* FILE: ztrmm_test.c 
* Purpose: to test BLAS function ZTRMM.
******************************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <time.h>
#include <complex.h>
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
double complex alpha = 0.7 - 0.3*I; 
enum CBLAS_ORDER     order  = CblasColMajor; 
enum CBLAS_TRANSPOSE transA = CblasNoTrans;
enum CBLAS_SIDE side  = CblasLeft;
enum CBLAS_UPLO uplo  = CblasUpper;
enum CBLAS_DIAG diag  = CblasUnit;

extern int TI_CBLAS_L3_OFFLOAD;

/*-----------------------------------------------------------------------------
* Prototypes
*----------------------------------------------------------------------------*/
int check_results(const double complex *C1, const double complex *C2, int M, int N);
int run_ztrmm(int M, int N, float *time, float *gflops);

/*-----------------------------------------------------------------------------
* MAIN
*----------------------------------------------------------------------------*/
int main()
{
    int ztrmm_err;
    int M, N, m, n;
    float time_secs, gflops;
    FILE *fp_time, *fp_gflops;  
  
    fp_time = fopen("ztrmm_time.dat","w");
    fp_gflops = fopen("ztrmm_gflops.dat","w");
    
    srand(12345);
    
    /* setting up TI CBLAS during first call */
    run_ztrmm(1000, 1000, &time_secs, &gflops);
    
    /* sweep M, and N */    
    for (M=START_SIZE_RECTAN_MATRIX,m=0; m<NUM_MATRIX_SIZE_TO_TEST; m++,M*=2) 
    {
        for (N=START_SIZE_RECTAN_MATRIX,n=0; n<NUM_MATRIX_SIZE_TO_TEST; n++,N*=2) 
        {    
            printf("Running ZTRMM for (M,N)=(%d,%d)\n", M,N);
            ztrmm_err = run_ztrmm(M, N, &time_secs, &gflops);
          
            if(ztrmm_err == -1) {  /* out of memory for DSP offloading */
                printf("Out of memory for (M,N) = (%d,%d).\n", M,N);
                fprintf(fp_time, "%6d\t%6d\t%10.8e\n", M, N, -1.0);
                fprintf(fp_gflops, "%6d\t%6d\t%10.8e\n", M, N, -1.0);
            }
            else {
                fprintf(fp_time, "%6d\t%6d\t%10.8e\n", M, N, time_secs);
                fprintf(fp_gflops, "%6d\t%6d\t%10.8e\n", M, N, gflops);
            }
        }
    }
       
    fclose(fp_time);
    fclose(fp_gflops);
    
    printf("Passed.\n");
	
    return 0;
}

int run_ztrmm(int M, int N, float *time, float *gflops)
{
    long long i, size_A, size_B;
    int   iter;
    float time_secs, total_time, total_gflops, gflops_iter;
    float operation_count;
    int   err_code = 0;

    total_time= 0.0;
	total_gflops = 0.0;
	if(side == CblasLeft) {
	  size_A = (long long)M*(long long)M;
      operation_count = 4.0*(double)M*(double)M*(double)N;    
	}
	else {
	  size_A = (long long)N*(long long)N;
      operation_count = 4.0*(double)M*(double)N*(double)N;    
	}
	size_B = (long long)M*(long long)N;
    if(  (size_A*sizeof(double complex)>(long long)0x0ffffffff) 
       ||(size_B*sizeof(double complex)>(long long)0x0ffffffff) ) {
        return (-1);
    }
	
    for (iter = 0; iter < NUM_TEST_RUN; iter++)
    {      
      /*-------------------------------------------------------------------------
      * Allocate space for the matrices.  The matrices that will be passed to 
      * the DSP are allocated using device memory.  
      *------------------------------------------------------------------------*/
      double complex *A = (double complex *) __malloc_ddr(size_A*sizeof(double complex));
      double complex *B = (double complex *) __malloc_ddr(size_B*sizeof(double complex));
  
      if (!A || !B)
      {
          printf("Could not allocate enough space for the arrays!");
          if(A) __free_ddr(A);
          if(B) __free_ddr(B);
          
          return (-1);
      }
  
      /*-------------------------------------------------------------------------
      * Initialize matrices 
      *------------------------------------------------------------------------*/
      int lda = (side == CblasLeft) ? M : N;  
      int ldb = M;  
      for (i = 0; i < size_A; ++i) 
	  {
          A[i] = (double)rand()/RAND_MAX + (double)rand()/RAND_MAX * I;
	  }

      for (i = 0; i < size_B; ++i)
	  {
          B[i] = (double)rand()/RAND_MAX + (double)rand()/RAND_MAX * I;
      }
    
      /*------------------------------------------------------------------------
      * Time ztrmm
      *-----------------------------------------------------------------------*/     
      tick();
      cblas_ztrmm(order,side,uplo,transA,diag,M,N,&alpha,A,lda,B,ldb);
      time_secs = tock();
      total_time += time_secs;
      gflops_iter = operation_count/time_secs*1e-9;
      total_gflops += gflops_iter;
  
      __free_ddr(A);
      __free_ddr(B);
    }
    
    *gflops = total_gflops / (double)NUM_TEST_RUN;
    *time   = total_time / (double)NUM_TEST_RUN;
    
    return err_code;
}


