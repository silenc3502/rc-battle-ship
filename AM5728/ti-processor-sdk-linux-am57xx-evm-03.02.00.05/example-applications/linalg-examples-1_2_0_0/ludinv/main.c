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
* FILE: ludinv/main.c 
* Purpose: to demonstrate how to do matrix inversion through LU decomposition 
*          using LAPACK functions dgetrf_() and dgetri_().
******************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <CL/cl_ext.h>

#ifdef __cplusplus
extern "C" {
#endif
#include "cblas.h"
#include "f2c.h"
#include "blaswrap.h"
#include "clapack.h"
#ifdef __cplusplus
}
#endif

/*-----------------------------------------------------------------------------
* Timing Setup
*----------------------------------------------------------------------------*/
struct timespec t0,t1;
#define tick()  clock_gettime(CLOCK_MONOTONIC, &t0);
#define tock() (clock_gettime(CLOCK_MONOTONIC, &t1), \
                        t1.tv_sec - t0.tv_sec + (t1.tv_nsec - t0.tv_nsec) / 1e9)

/* Auxiliary routines prototypes */
void print_int_vector( char* desc, integer n, integer* a );
void print_real_matrix(char* desc, doublereal *mat, integer n, integer m, integer ldv ); 
double compute_inverse_error(doublereal *matrix_A, doublereal *matrix_A_inverse, integer n, integer ld);
void print_performance(integer *matrix_sizes, float *secs_runs, int num_test, int num_run);
extern int dlatmr_(integer *m, integer *n, char *dist, integer *
	iseed, char *sym, doublereal *d__, integer *mode, doublereal *cond, 
	doublereal *dmax__, char *rsign, char *grade, doublereal *dl, integer 
	*model, doublereal *condl, doublereal *dr, integer *moder, doublereal 
	*condr, char *pivtng, integer *ipivot, integer *kl, integer *ku, 
	doublereal *sparse, doublereal *anorm, char *pack, doublereal *a, 
	integer *lda, integer *iwork, integer *info);

/* Parameters */
#define MAX_ALLOWED_ERR (1e-10)

/* Main program */
int print_data;
int main(int argc, char *argv[]) {
    /* Locals */
    float secs, secs_lud_inv, *secs_all_runs;
    int num_test, num_run, i, j;
    integer n, n_min, n_inc, info, lwork, lda;
    double max_abs_error;
    doublereal lwork_opt, *work, *matrix_A, *matrix_LUD;
    doublereal *vec_dbr_diagr, *vec_dbr_diag, *vec_dbr_diagl;
    integer    *vec_int_pivot, *vec_int_work;
    integer    *matrix_sizes;
    
    integer const_int6    = 6;
    doublereal const_dbr1 = 1.;
    doublereal const_dbr0 = 0.;
    integer rand_seed[4] = {10,123,278,3579};  /* numbers must lie between 0 and 4095 */

    fprintf(stdout, "\n\n===== LAPACK example: matrix inversion through LUD =====\n");
    if(argc == 1) { /* no command line arguments, use default */
        num_test = 3;
        n_min    = 1000;
        n_inc    = 500;
        num_run  = 1;
        print_data = 0;
    }
    else if (argc < 6) {
        printf("Usage: ludinv <num_test> <start_size> <size_inc> <num_run per size> <print data or not (1 or 0)>\n");
        exit(0);
    }
    else {
        num_test = atoi(argv[1]);
        n_min    = atoi(argv[2]);
        n_inc    = atoi(argv[3]);
        num_run  = atoi(argv[4]);
        print_data = atoi(argv[5]);
    }
    rand_seed[0] = 4675;
    
    secs_all_runs = (float *)malloc(sizeof(float)*num_test*num_run);
    matrix_sizes  = (integer*)malloc(sizeof(integer)*num_test);
    
    for(i=0, n=n_min; i<num_test; i++, n+=n_inc) 
    {
        lda = n;
        matrix_sizes[i] = n;

        printf("\n\nStart matrix inversion by LUD for matrix size %d. \n", (int)n);
        
        /* Operation counts of LUD inversion according to LAPACK Working Note 41 */
        /*operation_counts = 2.*n*n*n - 1.5*n*n + 2.5*n;  */
        
        /* Allocate memory for matrix and vectors */
        matrix_A      = (doublereal *)__malloc_ddr(sizeof(doublereal)*n*n);
        matrix_LUD    = (doublereal *)__malloc_ddr(sizeof(doublereal)*n*n);
        vec_int_pivot = (integer    *)__malloc_ddr(sizeof(integer)*n);     
        vec_int_work  = (integer    *)__malloc_ddr(sizeof(integer)*n);     
        vec_dbr_diag  = (doublereal *)__malloc_ddr(sizeof(doublereal)*n);  
        vec_dbr_diagl = (doublereal *)__malloc_ddr(sizeof(doublereal)*n);  
        vec_dbr_diagr = (doublereal *)__malloc_ddr(sizeof(doublereal)*n);  
                
        j = 0;
        while(j<num_run) {
            /* Generate a N by N random matrix */
            printf("\nStart test run %d out of %d. \n", (int)(j+1), (int)num_run);
            printf("Generate %d by %d random matrix. \n", (int)n, (int)n);
            dlatmr_(&n, &n, "S", rand_seed, "N", vec_dbr_diag, &const_int6, &const_dbr1, 
                    &const_dbr1, "T", "N", vec_dbr_diagl, &const_int6, &const_dbr1, vec_dbr_diagr, 
                    &const_int6, &const_dbr1, "N", vec_int_pivot, &n, &n, &const_dbr0,
                    &const_dbr1, "N", matrix_A, &lda, vec_int_work, &info);
            if(info != 0) {
                printf("Random matrix generation has a problem. Returned error value is %d. Start a new run. \n", (int)info);
                //goto START_NEW_RUN;
                exit(-1);
            }
        
            memcpy((void *)matrix_LUD, (void *)matrix_A, sizeof(doublereal)*n*n);
            
            print_real_matrix("Generated random matrix (double real): ", matrix_LUD, n, n, lda);

            tick();
            
            /* Computes LU factorization */
            dgetrf_(&n, &n, matrix_LUD, &lda, vec_int_pivot, &info);
            
            secs_lud_inv = tock();
            
            if(info != 0) {
                printf("LU factorization has a problem. Returned error value of dgetrf is %d. Start a new run. \n", (int)info);
                //goto START_NEW_RUN;
                exit(-1);
            }
            
            printf("LU decomposition finished. Time spent on LUD: %f seconds.\n", secs_lud_inv);
            
            /* Print details of LU factorization */
            print_real_matrix( "Details of LU factorization", matrix_LUD, n, n, lda);
        
            /* Print pivot indices */
            print_int_vector( "Pivot indices", n, vec_int_pivot );
        
            tick();
            
            /* Query and allocate the optimal workspace */
            lwork = -1;
            dgetri_(&n, matrix_LUD, &lda, vec_int_pivot, &lwork_opt, &lwork, &info);        
            lwork = (integer)lwork_opt;
            work = (doublereal *)__malloc_ddr( lwork*sizeof(doublereal) );
            
            /* Compute inversion of matrix based on LU factorization */
            dgetri_(&n, matrix_LUD, &lda, vec_int_pivot, work, &lwork, &info);    
            
            secs = tock();
            
            __free_ddr((void *)work);
            if(info != 0) {
                printf("Matrix inversion through LU factorization has a problem.");
                printf("Returned error value of dgetri is %d. Start a new run. \n", (int)info);
                //goto START_NEW_RUN;
                exit(-1);
            }
                    
            secs_lud_inv += secs;
            secs_all_runs[i*num_run+j] = secs_lud_inv;
            printf("Matrix inversion finished. Time spent on inversion: %f seconds.\n", secs);
            printf("Time spent on LUD and inversion: %f seconds.\n", secs_lud_inv);
           
            /* Print solution */
            print_real_matrix( "Inverse of matrix A through LU factorization:", matrix_LUD, n, n, lda);        
        
            /* Compute A*inv(A) and precision error: I-A*inv(A) */
            max_abs_error = compute_inverse_error(matrix_A, matrix_LUD, n, lda);
            printf("Maximum error of matrix inversion by LUD: %e\n", max_abs_error);
            
            if(max_abs_error > MAX_ALLOWED_ERR) {
                printf("Error is out of tolerance range.\n");
                exit(-1);
            }

            j++;
            
START_NEW_RUN:
            printf("Current run finished. \n");
        } /* while(j<num_run) */
        
        __free_ddr((void *)matrix_A);
        __free_ddr((void *)matrix_LUD);
        __free_ddr((void *)vec_int_pivot);
        __free_ddr((void *)vec_int_work);
        __free_ddr((void *)vec_dbr_diag);
        __free_ddr((void *)vec_dbr_diagl);
        __free_ddr((void *)vec_dbr_diagr);        
    }  /* for(i=0, n=n_min; i<num_test; i++, n+=n_inc) */
    
    print_performance(matrix_sizes, secs_all_runs, num_test, num_run);
    
    printf("Passed.\n");

    exit( 0 );
} 


/* Auxiliary routine: printing a vector of integers */
void print_int_vector( char* desc, integer n, integer* a ) 
{
   int j;

   if (!print_data)
     return;
        
   printf( "\n %s\n", desc );
   for( j = 0; j < n; j++ ) printf( " %6d", (int)a[j] );
   printf( "\n" );
}

/* Auxiliary routine: printing a real matrix */
void print_real_matrix(char* desc, doublereal *mat, integer m, integer n, integer ldv ) 
{
   int i, j;

   if (!print_data)
     return;
     
   printf( "\n %s\n", desc );
   for(i=0; i<m; i++) {
      for(j=0; j<n; j++) {
         printf( " %10.5f ", mat[i+j*ldv]);
      }
      printf( "\n" );
  }
}


double compute_inverse_error(doublereal *matrix_A, doublereal *matrix_A_inverse, integer n, integer ld)
{
   int i, j;
   double abs_err, relative_err;
   doublereal *matrix_A_mult_A_inv;
   double max_err = 0.;

   matrix_A_mult_A_inv = (doublereal *)malloc(n*n*sizeof(doublereal));
   
   cblas_dgemm(CblasColMajor,CblasNoTrans,CblasNoTrans,n,n,n,1.0,matrix_A,ld,
               matrix_A_inverse,ld,0.0,matrix_A_mult_A_inv,ld);
   
   /* sqrt(sum(err^2))/N, sum(fabs(err))/N, max(fabs(err))  */
   for(i=0; i<n; i++) {
      for(j=0; j<n; j++) {
        if(i==j) {
          abs_err = fabs(matrix_A_mult_A_inv[i+j*ld]-1.);
        }
        else {
          abs_err = fabs(matrix_A_mult_A_inv[i+j*ld]);
        }       
       
        if(abs_err > max_err) {
          max_err = abs_err;
        }
      }
   }
      
   free(matrix_A_mult_A_inv);
   return max_err;
}


void print_performance(integer *matrix_sizes, float *secs_runs, int num_test, int num_run)
{
    int i, j;
    float inst, min, max;
    float tot, avg;
    
    printf("Seconds for all matrix sizes:\n");
    printf("Matrix size   ");
    printf("    Min           Max        Average  ");
    printf("\n");
    
    for(i=0; i<num_test; i++)
    {
        printf("%11d ", (int)matrix_sizes[i]);
        tot = 0.;
        max = 0.;
        min = 1e100;
        for(j=0; j<num_run; j++) 
          {
            inst = secs_runs[i*num_run+j];            
            tot += inst;
            
            if(inst>max) max = inst;
            if(inst<min) min = inst;
          }
        avg = tot/(float)num_run;
        printf("%12f%12f%12f\n", min, max, avg);
    }
}
