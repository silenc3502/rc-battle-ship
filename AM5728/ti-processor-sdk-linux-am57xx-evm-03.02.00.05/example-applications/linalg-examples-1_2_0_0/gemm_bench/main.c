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
* FILE: gemm_bench/main.c 
* Purpose: to benchmark BLAS functions xGEMM.
******************************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <time.h>
#include <CL/cl.h>
#include <CL/cl_ext.h>
#include <complex.h>

#include "cblas.h"

#define GEMM_MATRIX_SIZE_START 1024
#define NUM_MATRIX_SIZE_TO_BENCHMARK 2
#define NUM_TESTS (NUM_MATRIX_SIZE_TO_BENCHMARK*NUM_MATRIX_SIZE_TO_BENCHMARK*NUM_MATRIX_SIZE_TO_BENCHMARK)
#define HAS_MEMORY   1
#define NO_MEMORY    0
#define OFFLOAD      1
#define NO_OFFLOAD   0

#define NUM_TEST_RUN 5

#define GFLOPS_MARGIN (1.1f)

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
enum CBLAS_ORDER     order  = CblasColMajor; 
enum CBLAS_TRANSPOSE transA = CblasNoTrans;
enum CBLAS_TRANSPOSE transB = CblasNoTrans;

extern int TI_CBLAS_L3_OFFLOAD;
/*-----------------------------------------------------------------------------
* Prototypes
*----------------------------------------------------------------------------*/
int run_dgemm(int M, int N, int K, float *time, float *gflops);
int run_sgemm(int M, int N, int K, float *time, float *gflops);
int run_cgemm(int M, int N, int K, float *time, float *gflops);
int run_zgemm(int M, int N, int K, float *time, float *gflops);

/* reference GFLOPS based on 1GHz K2H device */
float dgemm_gflops_ref[NUM_TESTS] = 
{21.6774,21.9383,22.3325,22.7754,22.6200,23.0515,23.3946,23.6324};

float sgemm_gflops_ref[NUM_TESTS] = 
{62.8003,70.5709,69.2643,73.3178,71.8381,75.6127,74.2427,80.8945};

float cgemm_gflops_ref[NUM_TESTS] = 
{78.5054,78.8472,80.2723,82.3480,82.2609,84.3821,85.6639,86.9018};

float zgemm_gflops_ref[NUM_TESTS] = 
{13.9430,13.8067,14.0683,13.8591,14.0504,13.9755,14.1071,13.9960};

/*-----------------------------------------------------------------------------
* MAIN
*----------------------------------------------------------------------------*/
int main()
{
    int num_size, gemm_err;
    int M, N, K, m, n, k, test_idx;
    float time_secs, gflops, gflops_ref, cpu_freq_GHz;
    cl_platform_id platform;
    cl_uint num_platforms;
    cl_device_id devices;
    cl_uint num_devices;
    cl_uint cpu_freq;
    size_t cpu_freq_size;
    FILE *fp_time, *fp_gflops;  

    if(clGetPlatformIDs(1, &platform, &num_platforms) != CL_SUCCESS) {
      printf("Error in clGetPlatformIDs\n.");
      exit(0);
    }
  
    if(clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, 1, &devices, &num_devices) != CL_SUCCESS) {
      printf("Error in clGetDeviceIDs\n.");
      exit(0);
    }
    if(clGetDeviceInfo(devices, CL_DEVICE_MAX_CLOCK_FREQUENCY, sizeof(cl_uint), (void *)&cpu_freq, &cpu_freq_size) != CL_SUCCESS) {
      printf("Error in clGetDeviceInfo\n.");
      exit(0);   
    }
    cpu_freq_GHz = (float)cpu_freq/1e3; /* convert from MHz to GHz */
    printf("CPU frequency is %f GHz.\n", cpu_freq_GHz);
 
    srand(12345);
    
    /* setting up TI CBLAS during first call */
    run_dgemm(1000, 1000, 1000, &time_secs, &gflops);
 
    /*------- benchmarking DGEMM ------- */    
    fp_time = fopen("dgemm_time.dat","w");
    fp_gflops = fopen("dgemm_gflops.dat","w");
 
    test_idx = 0;
    for (M=GEMM_MATRIX_SIZE_START,m=0; m<NUM_MATRIX_SIZE_TO_BENCHMARK; m++,M*=2) 
    {
        for (N=GEMM_MATRIX_SIZE_START,n=0; n<NUM_MATRIX_SIZE_TO_BENCHMARK; n++,N*=2) 
        {
            for (K=GEMM_MATRIX_SIZE_START,k=0; k<NUM_MATRIX_SIZE_TO_BENCHMARK; k++,K*=2) 
            {
                printf("Running DGEMM for (M,N,K) = (%d,%d,%d). ", M,N,K);
    
                gemm_err = run_dgemm(M, N, K, &time_secs, &gflops);
          
                if(gemm_err == -1) {  /* out of memory for DSP offloading */
                    printf("DGEMM out of memory for (M,N,K) = (%d,%d,%d).\n", M,N,K);
                    exit(0);
                }
                else {
                    fprintf(fp_time,  "%6d\t%6d\t%6d\t%10.8e\n", M, N, K, time_secs);
                    fprintf(fp_gflops, "%6d\t%6d\t%6d\t%10.8e\n", M, N, K, gflops);
                }
    
                gflops_ref  = dgemm_gflops_ref[test_idx++]; /* read reference GFLOPS */
                gflops_ref  = gflops_ref * cpu_freq_GHz;    /* scale ref GFLOPS by CPU freq */
                printf("Measured %f GFLOPS, reference %f GFLOPS.\n", gflops, gflops_ref);
                if((gflops > gflops_ref*GFLOPS_MARGIN) || (gflops < gflops_ref/GFLOPS_MARGIN)) {
                    printf("DGEMM test FAILED! GFLOPS deviates from reference unacceptably.\n");
                    exit(0);
                }
          
                if(gemm_err == -1) {  /* out of memory for DSP offloading */
                    printf("Out of memory for (M,N,K) = (%d,%d,%d).\n", M,N,K);
                    exit(0);
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

    /*------- benchmarking SGEMM -------*/    
    fp_time = fopen("sgemm_time.dat","w");
    fp_gflops = fopen("sgemm_gflops.dat","w");
 
    test_idx = 0;
    for (M=GEMM_MATRIX_SIZE_START,m=0; m<NUM_MATRIX_SIZE_TO_BENCHMARK; m++,M*=2) 
    {
        for (N=GEMM_MATRIX_SIZE_START,n=0; n<NUM_MATRIX_SIZE_TO_BENCHMARK; n++,N*=2) 
        {
            for (K=GEMM_MATRIX_SIZE_START,k=0; k<NUM_MATRIX_SIZE_TO_BENCHMARK; k++,K*=2) 
            {
                printf("Running SGEMM for (M,N,K) = (%d,%d,%d). ", M,N,K);
    
                gemm_err = run_sgemm(M, N, K, &time_secs, &gflops);
          
                if(gemm_err == -1) {  /* out of memory for DSP offloading */
                    printf("SGEMM out of memory for (M,N,K) = (%d,%d,%d).\n", M,N,K);
                    exit(0);
                }
                else {
                    fprintf(fp_time,  "%6d\t%6d\t%6d\t%10.8e\n", M, N, K, time_secs);
                    fprintf(fp_gflops, "%6d\t%6d\t%6d\t%10.8e\n", M, N, K, gflops);
                }
    
                gflops_ref  = sgemm_gflops_ref[test_idx++]; /* read reference GFLOPS */
                gflops_ref  = gflops_ref * cpu_freq_GHz;    /* scale ref GFLOPS by CPU freq */
                printf("Measured %f GFLOPS, reference %f GFLOPS.\n", gflops, gflops_ref);
                if((gflops > gflops_ref*GFLOPS_MARGIN) || (gflops < gflops_ref/GFLOPS_MARGIN)) {
                    printf("SGEMM test FAILED! GFLOPS deviates from reference unacceptably.\n");
                    exit(0);
                }
          
                if(gemm_err == -1) {  /* out of memory for DSP offloading */
                    printf("Out of memory for (M,N,K) = (%d,%d,%d).\n", M,N,K);
                    exit(0);
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

    /*------- benchmarking CGEMM -------*/    
    fp_time = fopen("cgemm_time.dat","w");
    fp_gflops = fopen("cgemm_gflops.dat","w");
 
    test_idx = 0;
    for (M=GEMM_MATRIX_SIZE_START,m=0; m<NUM_MATRIX_SIZE_TO_BENCHMARK; m++,M*=2) 
    {
        for (N=GEMM_MATRIX_SIZE_START,n=0; n<NUM_MATRIX_SIZE_TO_BENCHMARK; n++,N*=2) 
        {
            for (K=GEMM_MATRIX_SIZE_START,k=0; k<NUM_MATRIX_SIZE_TO_BENCHMARK; k++,K*=2) 
            {
                printf("Running CGEMM for (M,N,K) = (%d,%d,%d). ", M,N,K);
    
                gemm_err = run_cgemm(M, N, K, &time_secs, &gflops);
          
                if(gemm_err == -1) {  /* out of memory for DSP offloading */
                    printf("CGEMM out of memory for (M,N,K) = (%d,%d,%d).\n", M,N,K);
                    exit(0);
                }
                else {
                    fprintf(fp_time,  "%6d\t%6d\t%6d\t%10.8e\n", M, N, K, time_secs);
                    fprintf(fp_gflops, "%6d\t%6d\t%6d\t%10.8e\n", M, N, K, gflops);
                }
    
                gflops_ref  = cgemm_gflops_ref[test_idx++]; /* read reference GFLOPS */
                gflops_ref  = gflops_ref * cpu_freq_GHz;    /* scale ref GFLOPS by CPU freq */
                printf("Measured %f GFLOPS, reference %f GFLOPS.\n", gflops, gflops_ref);
                if((gflops > gflops_ref*GFLOPS_MARGIN) || (gflops < gflops_ref/GFLOPS_MARGIN)) {
                    printf("CGEMM test FAILED! GFLOPS deviates from reference unacceptably.\n");
                    exit(0);
                }
            }
        }
    }
        
    fclose(fp_time);
    fclose(fp_gflops);

    /*------- benchmarking ZGEMM -------*/    
    fp_time = fopen("zgemm_time.dat","w");
    fp_gflops = fopen("zgemm_gflops.dat","w");
 
    test_idx = 0;
    for (M=GEMM_MATRIX_SIZE_START,m=0; m<NUM_MATRIX_SIZE_TO_BENCHMARK; m++,M*=2) 
    {
        for (N=GEMM_MATRIX_SIZE_START,n=0; n<NUM_MATRIX_SIZE_TO_BENCHMARK; n++,N*=2) 
        {
            for (K=GEMM_MATRIX_SIZE_START,k=0; k<NUM_MATRIX_SIZE_TO_BENCHMARK; k++,K*=2) 
            {
                printf("Running ZGEMM for (M,N,K) = (%d,%d,%d). ", M,N,K);
    
                gemm_err = run_zgemm(M, N, K, &time_secs, &gflops);
                if(gemm_err == -1) {  /* out of memory for DSP offloading */
                    printf("ZGEMM out of memory for (M,N,K) = (%d,%d,%d).\n", M,N,K);
                    exit(0);
                }
                else {
                    fprintf(fp_time,  "%6d\t%6d\t%6d\t%10.8e\n", M, N, K, time_secs);
                    fprintf(fp_gflops, "%6d\t%6d\t%6d\t%10.8e\n", M, N, K, gflops);
                }
    
                gflops_ref  = zgemm_gflops_ref[test_idx++]; /* read reference GFLOPS */
                gflops_ref  = gflops_ref * cpu_freq_GHz;    /* scale ref GFLOPS by CPU freq */
                printf("Measured %f GFLOPS, reference %f GFLOPS.\n", gflops, gflops_ref);
                if((gflops > gflops_ref*GFLOPS_MARGIN) || (gflops < gflops_ref/GFLOPS_MARGIN)) {
                    printf("ZGEMM test FAILED! GFLOPS deviates from reference unacceptably.\n");
                    exit(0);
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
    float time_secs, total_time;
    float operation_count = 2.0*(float)M*(float)N*(float)K;
    float total_GFLOPS = 0.0f;
    double alpha = 0.7; 
    double beta  = 0.3;
    int    err_code = 0;
    
    /*-------------------------------------------------------------------------
    * Allocate space for the matrices.  
    *------------------------------------------------------------------------*/
    double *A = (double *) __malloc_ddr((long long)M*(long long)K*(long long)sizeof(double));
    double *B = (double *) __malloc_ddr((long long)K*(long long)N*(long long)sizeof(double));
    double *C = (double *) __malloc_ddr((long long)M*(long long)N*(long long)sizeof(double));
  
    if (!A || !B || !C)
    {
        printf("Could not allocate enough space for the matrices!");
        if(A) __free_ddr(A);
        if(B) __free_ddr(B);
        if(C) __free_ddr(C);
        
        return (-1);
    }
   
    total_time = 0.0;
    for (iter = 0; iter < NUM_TEST_RUN; iter++)
    {        
        /*----------------------------------------------------------------------
        * Initialize matrices 
        *---------------------------------------------------------------------*/
        for (i = 0; i < (long long)M*K; ++i) A[i] = (double)rand()/RAND_MAX;
        for (i = 0; i < (long long)K*N; ++i) B[i] = (double)rand()/RAND_MAX;
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
    }

    __free_ddr(A);
    __free_ddr(B);
    __free_ddr(C);
    
    *gflops = total_GFLOPS / (float)NUM_TEST_RUN;
    *time   = total_time / (float)NUM_TEST_RUN;
    
    return err_code;
}

int run_sgemm(int M, int N, int K, float *time, float *gflops)
{
    int   iter;
    long long i;
    float time_secs, total_time;
    float operation_count = 2.0*(float)M*(float)N*(float)K;
    float total_GFLOPS = 0.0f;
    float alpha = 0.7; 
    float beta  = 0.3;
    int   err_code = 0;
    
    /*-------------------------------------------------------------------------
    * Allocate space for the matrices.  
    *------------------------------------------------------------------------*/
    float *A = (float *) __malloc_ddr((long long)M*(long long)K*(long long)sizeof(float));
    float *B = (float *) __malloc_ddr((long long)K*(long long)N*(long long)sizeof(float));
    float *C = (float *) __malloc_ddr((long long)M*(long long)N*(long long)sizeof(float));
  
    if (!A || !B || !C)
    {
        printf("Could not allocate enough space for the matrices!");
        if(A) __free_ddr(A);
        if(B) __free_ddr(B);
        if(C) __free_ddr(C);
        
        return (-1);
    }
   
    total_time = 0.0;
    for (iter = 0; iter < NUM_TEST_RUN; iter++)
    {        
        /*----------------------------------------------------------------------
        * Initialize matrices 
        *---------------------------------------------------------------------*/
        for (i = 0; i < (long long)M*K; ++i) A[i] = (float)rand()/RAND_MAX;
        for (i = 0; i < (long long)K*N; ++i) B[i] = (float)rand()/RAND_MAX;
        for (i = 0; i < (long long)M*N; ++i) C[i] = (float)rand()/RAND_MAX;
    
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
        cblas_sgemm(order,transA,transB,M,N,K,alpha,A,lda,B,ldb,beta,C,ldc);
        time_secs = tock();
        total_time += time_secs;
        total_GFLOPS += operation_count/time_secs*1e-9;  
    }

    __free_ddr(A);
    __free_ddr(B);
    __free_ddr(C);
    
    *gflops = total_GFLOPS / (float)NUM_TEST_RUN;
    *time   = total_time / (float)NUM_TEST_RUN;
    
    return err_code;
}

int run_cgemm(int M, int N, int K, float *time, float *gflops)
{
    float complex alpha = 0.7 - 0.3*I; 
    float complex beta  = 0.4 + 0.6*I;
    int   iter;
    long long i;
    float time_secs, total_time;
    float operation_count = 2.0*(float)M*(float)N*(float)K*4.0;
    float total_GFLOPS = 0.0f;
    int   err_code = 0;
    
    /*-------------------------------------------------------------------------
    * Allocate space for the matrices.  
    *------------------------------------------------------------------------*/
    float complex *A = (float complex*) __malloc_ddr(M*K*sizeof(float complex));
    float complex *B = (float complex*) __malloc_ddr(K*N*sizeof(float complex));
    float complex *C = (float complex*) __malloc_ddr(M*N*sizeof(float complex));

    if (!A || !B || !C)
    {
        printf("Could not allocate enough space for the matrices!");
        if(A) __free_ddr(A);
        if(B) __free_ddr(B);
        if(C) __free_ddr(C);
        
        return (-1);
    }

    total_time = 0.0;
    for (iter = 0; iter < NUM_TEST_RUN; iter++)
    {      
        /*----------------------------------------------------------------------
        * Initialize matrices 
        *---------------------------------------------------------------------*/
        for (i = 0; i < M*K; ++i) 
        {
            A[i] = (float)rand()/RAND_MAX + (float)rand()/RAND_MAX * I;
        }
        for (i = 0; i < K*N; ++i)
        {
            B[i] = (float)rand()/RAND_MAX + (float)rand()/RAND_MAX * I;
        }
        for (i = 0; i < M*N; ++i)
        {
            C[i] = (float)rand()/RAND_MAX + (float)rand()/RAND_MAX * I;
        }  
    
        int lda = ((order == CblasColMajor && transA == CblasNoTrans) ||
                (order == CblasRowMajor && transA == CblasTrans)) ? M : K;
    
        int ldb = ((order == CblasColMajor && transB == CblasNoTrans) ||
                (order == CblasRowMajor && transB == CblasTrans)) ? K : N;
    
        int ldc = (order == CblasColMajor) ? M : N;
      
        /*----------------------------------------------------------------------
        * Run and time cgemm
        *---------------------------------------------------------------------*/
        tick();
        cblas_cgemm(order,transA,transB,M,N,K,&alpha,A,lda,B,ldb,&beta,C,ldc);
        time_secs = tock();  
        total_time += time_secs;
        total_GFLOPS += operation_count/time_secs*1e-9;  
    }
 
    __free_ddr(A);
    __free_ddr(B);
    __free_ddr(C);
    
    *gflops = total_GFLOPS / (float)NUM_TEST_RUN;
    *time   = total_time / (float)NUM_TEST_RUN;

    return err_code;
}

int run_zgemm(int M, int N, int K, float *time, float *gflops)
{
    double complex alpha = 0.7 - 0.3*I; 
    double complex beta  = 0.4 + 0.6*I;
    int   iter;
    long long i;
    float time_secs, total_time;
    float operation_count = 2.0*(float)M*(float)N*(float)K*4.0;
    float total_GFLOPS = 0.0f;
    int   err_code = 0;
    
    /*-------------------------------------------------------------------------
    * Allocate space for the matrices.  
    *------------------------------------------------------------------------*/
    double complex *A = (double complex*) __malloc_ddr(M*K*sizeof(double complex));
    double complex *B = (double complex*) __malloc_ddr(K*N*sizeof(double complex));
    double complex *C = (double complex*) __malloc_ddr(M*N*sizeof(double complex));

    if (!A || !B || !C)
    {
        printf("Could not allocate enough space for the matrices!");
        if(A) __free_ddr(A);
        if(B) __free_ddr(B);
        if(C) __free_ddr(C);
        
        return (-1);
    }

    total_time = 0.0;
    for (iter = 0; iter < NUM_TEST_RUN; iter++)
    {      
        /*----------------------------------------------------------------------
        * Initialize matrices 
        *---------------------------------------------------------------------*/
        for (i = 0; i < M*K; ++i) 
        {
            A[i] = (double)rand()/RAND_MAX + (double)rand()/RAND_MAX * I;
        }
        for (i = 0; i < K*N; ++i)
        {
            B[i] = (double)rand()/RAND_MAX + (double)rand()/RAND_MAX * I;
        }
        for (i = 0; i < M*N; ++i)
        {
            C[i] = (double)rand()/RAND_MAX + (double)rand()/RAND_MAX * I;
        }  
    
        int lda = ((order == CblasColMajor && transA == CblasNoTrans) ||
                (order == CblasRowMajor && transA == CblasTrans)) ? M : K;
    
        int ldb = ((order == CblasColMajor && transB == CblasNoTrans) ||
                (order == CblasRowMajor && transB == CblasTrans)) ? K : N;
    
        int ldc = (order == CblasColMajor) ? M : N;
      
        /*----------------------------------------------------------------------
        * Run and time zgemm
        *---------------------------------------------------------------------*/
        tick();
        cblas_zgemm(order,transA,transB,M,N,K,&alpha,A,lda,B,ldb,&beta,C,ldc);
        time_secs = tock();  
        total_time += time_secs;
        total_GFLOPS += operation_count/time_secs*1e-9;  
    }
 
    __free_ddr(A);
    __free_ddr(B);
    __free_ddr(C);
    
    *gflops = total_GFLOPS / (float)NUM_TEST_RUN;
    *time   = total_time / (float)NUM_TEST_RUN;

    return err_code;
}
