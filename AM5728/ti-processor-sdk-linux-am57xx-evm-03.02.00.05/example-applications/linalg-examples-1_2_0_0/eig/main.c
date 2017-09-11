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
* FILE: eig/main.c 
* Purpose: to demonstrate how to do Eigen decomposition and matrix inversion:
           A*P = P*D, and A^-1 = P*D*P^-1
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
void print_eigenvalues(char * desc, int n, double * vector_r, double * vector_i);
void print_eigenvectors(char * desc, int n, double * vector_i, double * v, int ldv);
void form_diag_matrix(doublecomplex *matrix_diag, doublereal *vector_r, doublereal *vector_i, int n);
void form_eigen_matrix(doublecomplex *mat, int n, double* vector_i, double* v, int ldv); 
void print_real_matrix(char* desc, doublereal *mat, int n, int m, int ldv); 
void print_complex_matrix(char* desc, doublecomplex *mat, int n, int m, int ldv);
doublereal compute_evd_error(doublecomplex *matrix_reconstruct, doublereal *matrix_original, int n, int ld);
void print_performance(char * func_name, int *matrix_sizes, float *time, int num_test, int num_run);
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
    
int main(int argc, char *argv[]) 
{
    /* Locals */
    float time, time_ev, time_cinv, *time_ev_all_runs, *time_cinv_all_runs;
    integer num_test, num_run, i, j, n, n_min, n_inc;
    integer lda, ldl, ldr, info, lwork;
    doublereal wkopt;
    doublereal* work;
    integer *vector_pivot, *vector_work;
	doublereal max_error_evd;
    doublereal *matrix_A, *matrix_A_cpy, *matrix_R, *matrix_L;
	doublereal *vector_diag, *vector_diagl, *vector_diagr, *vector_r, *vector_i;
    doublecomplex *matrix_D, *matrix_P, *matrix_invP, *matrix_A_cmplx, *work_dcomplex, lwork_opt;
    int *matrix_sizes;
    integer const_int6 = 6;
    doublereal const_dbr1 = 1.;
    doublereal const_dbr0 = 0.;       
    doublecomplex const_dbc1 = {1.,0.};
    doublecomplex const_dbc0 = {0.,0.};
	integer rand_seed[4] = {10,123,278,3579};  /* numbers must lie between 0 and 4095 */
    
    fprintf(stdout, "\n\n======= LAPACK example: eigen decomposition =======\n");
    if(argc == 1) { /* no command line arguments, use default */
        num_test = 3;
        n_min    = 100;
        n_inc    = 100;
        num_run  = 1;
        print_data = 0;
    }
    else if (argc < 6) {
        printf("Usage: eig <num_test> <start_size> <size_inc> <num_run per size> <print data or not (1 or 0)>\n");
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
    
    matrix_sizes  = (int*)malloc(sizeof(int)*num_test);
    time_ev_all_runs   = (float *)malloc(sizeof(float)*num_test*num_run);
    time_cinv_all_runs = (float *)malloc(sizeof(float)*num_test*num_run);
    
    for(i=0, n=n_min; i<num_test; i++, n+=n_inc) 
    {
        lda = ldl = ldr = n;
        matrix_sizes[i] = n;

        printf("\n\nStart EVD for matrix size %d. \n", (int)n);
    
        j = 0;
        while(j<num_run) 
        {
        
            /* Allocate memory for matrix and vectors */
            matrix_A     = (doublereal *)__malloc_ddr(sizeof(doublereal)*n*n);
            matrix_A_cpy = (doublereal *)malloc(sizeof(doublereal)*n*n);
            vector_pivot = (integer    *)__malloc_ddr(sizeof(integer)*n);     
            vector_work  = (integer    *)malloc(sizeof(integer)*n);     /* not used in the example */
            vector_diag  = (doublereal *)malloc(sizeof(doublereal)*n);  /* not used in the example */
            vector_diagl = (doublereal *)malloc(sizeof(doublereal)*n);  /* not used in the example */
            vector_diagr = (doublereal *)malloc(sizeof(doublereal)*n);  /* not used in the example */
            matrix_D     = (doublecomplex *)__malloc_ddr(n*n*sizeof(doublecomplex));
            matrix_P     = (doublecomplex *)__malloc_ddr(n*n*sizeof(doublecomplex));
            matrix_invP  = (doublecomplex *)__malloc_ddr(n*n*sizeof(doublecomplex));
            vector_r     = (doublereal *)__malloc_ddr(sizeof(doublereal)*n);
            vector_i     = (doublereal *)__malloc_ddr(sizeof(doublereal)*n);
            matrix_L     = (doublereal *)__malloc_ddr(sizeof(doublereal)*n*n); /* left eigenvectors */
            matrix_R     = (doublereal *)__malloc_ddr(sizeof(doublereal)*n*n); /* right eigenvectors */
        
            /* Generate a N by N random matrix using LAPACK auxiliary testing routine. */
            printf("\nStart test run %d out of %d. \n", (int)(j+1), (int)num_run);
            printf("Generate %d by %d random matrix. \n", (int)n, (int)n);
            dlatmr_(&n, &n, "S", rand_seed, "N", vector_diag, &const_int6, &const_dbr1, 
                    &const_dbr1, "T", "N", vector_diagl, &const_int6, &const_dbr1, vector_diagr, 
                    &const_int6, &const_dbr1, "N", vector_pivot, &n, &n, &const_dbr0,
                    &const_dbr1, "N", matrix_A, &n, vector_work, &info);
            //EDMA_Free();
            if(info != 0) {
                printf("Random matrix generation has a problem. Returned error value is %d. Start a new run. \n", (int)info);
                goto START_NEW_RUN;
            }
            
            print_real_matrix("Generated random matrix (double real): ", matrix_A, n, n, lda);
            memcpy((void *)matrix_A_cpy, (void *)matrix_A, sizeof(doublereal)*n*n);

            //time = read_cycle();
            tick();
            
            /* Query and allocate the optimal workspace for eigenproblem solution. */
            lwork = -1;
            dgeev_("N", "V", &n, matrix_A, &lda, vector_r, vector_i, matrix_L, &ldl, matrix_R, &ldr,
                   &wkopt, &lwork, &info);       /* to find right eigenvectors only */
        
            lwork = (int)wkopt;  /* get optimal workspace size returned by dgeev_() */
            work = (doublereal*)__malloc_ddr( lwork*sizeof(doublereal) );
        
            /* Find eigenvectors and eigenvalues: AP = PD, where colums of P are eigenvectors,
               and diagonal elements of D are eigenvalues. */
            dgeev_("N", "Vectors", &n, matrix_A, &lda, vector_r, vector_i, matrix_L, &ldl, matrix_R, &ldr,
                   work, &lwork, &info);        /* find right eigenvectors only */
            time_ev = tock();;
            time_ev_all_runs[i*num_run+j] = time_ev;
            
            printf("Perform eigen-decomposition such that AP = PD.\n");    
            
            __free_ddr( (void*)work );
                   
            /* Check for convergence */
            if( info > 0 ) {
                printf("The eigen solution algorithm failed to compute eigenvalues.");
                printf("To generate a new matrix to test the algorithm.\n");
                goto START_NEW_RUN;
            }
        
            printf("Eigen-decomposition finished successfully.\n");
            print_eigenvalues( "Eigenvalues", n, vector_r, vector_i );       
            print_eigenvectors( "Right eigenvectors", n, vector_i, matrix_R, ldr );
            
            /* Form matrix P using right eigenvectors as the columns. */
            form_eigen_matrix(matrix_P, n, vector_i, matrix_R, ldr);        
            print_complex_matrix("Eigenvector matrix P:", matrix_P, n, n, n);
            
            /* Form matrix D using eigenvalues as the diagonal elements. */
            form_diag_matrix(matrix_D, vector_r, vector_i, n);
            print_complex_matrix("Complex diagonal matrix D: ", matrix_D, n, n, n);    
        
            /* Compute inverse of P: first perform LU decomposition on P. */
            printf("Compute inverse of P such that A = PDP^-1.\n");    

            zlacpy_("Full", &n, &n, matrix_P, &n, matrix_invP, &n);

            tick();            
            zgetrf_(&n, &n, matrix_invP, &n, vector_pivot, &info);
            time_cinv = tock();  /* time for complex matrix inversion */
            
            if( info != 0 ) {
                printf("Return error value of zgetrf_(): %d.\n", (int)info);
                printf("The LU decomposition failed.");
                printf("To generate a new matrix to test the algorithm.\n");
                goto START_NEW_RUN;
            }        
            
            print_complex_matrix("matrix P LU decomposition results", matrix_invP, n, n, n);

            tick();
            
            /* Query and allocate the optimal workspace for matrix inversion 
               based on LU decompostion. */
            lwork = -1;
            zgetri_(&n, matrix_invP, &n, vector_pivot, &lwork_opt, &lwork, &info);    
            
            lwork = (int)lwork_opt.r; /* get optimal workspace size returned by zgetri_() */
            work_dcomplex = (doublecomplex *)malloc( lwork*sizeof(doublecomplex) );
            
            /* Compute inverse of P based on LUD results. */
            zgetri_(&n, matrix_invP, &n, vector_pivot, work_dcomplex, &lwork, &info);
            free( (void*)work_dcomplex );

            time_cinv += tock();
            time_cinv_all_runs[i*num_run+j] = time_cinv;
            
            if( info != 0 ) {
                printf("Return error value of zgetri_(): %d.\n", (int)info);
                printf("Matrix P inversion through LU decomposition failed.");
                printf("To generate a new matrix to test the algorithm.\n");
                goto START_NEW_RUN;
            }        
            printf("Matrix P inversion finished successfully.");
            print_complex_matrix("Inverse of eigenvector matrix P:", matrix_invP, n, n, n);

            /* Compute P*D*inv(P). */ 
            work_dcomplex = (doublecomplex *)malloc(sizeof(doublecomplex)*n*n);
            zgemm_("N", "N", &n, &n, &n, &const_dbc1, matrix_P, &n, matrix_D, &n, &const_dbc0, work_dcomplex, &n);
            //EDMA_Free();
            
            matrix_A_cmplx = matrix_P;  /* reuse memory */
            zgemm_("N", "N", &n, &n, &n, &const_dbc1, work_dcomplex, &n, matrix_invP, &n, &const_dbc0, matrix_A_cmplx, &n);
            print_complex_matrix("P*D*inv(P):", matrix_A_cmplx, n, n, n);
            print_real_matrix("Original matrix A", matrix_A_cpy, n, n, n);
            
            /* Compute error: A-P*D*inv(P) */
            max_error_evd = compute_evd_error(matrix_A_cmplx, matrix_A_cpy, n, lda);
            
            printf("\nMaximum error of EVD: %e\n", max_error_evd);

            if(max_error_evd > MAX_ALLOWED_ERR) {
                printf("Error is out of tolerance range.\n");
                exit(-1);
            }
            
            free( (void*)work_dcomplex );        

            j++;
            
START_NEW_RUN:
            printf("Current run finished. \n");
            
            /* Free workspace */
            __free_ddr((void *)matrix_A);
            __free_ddr((void *)vector_pivot);
            free((void *)vector_work );
            free((void *)vector_diag );
            free((void *)vector_diagl);
            free((void *)vector_diagr);
            __free_ddr((void *)vector_r);
            __free_ddr((void *)vector_i);
            __free_ddr((void *)matrix_L);
            __free_ddr((void *)matrix_R);            
            free((void *)matrix_A_cpy);
            __free_ddr((void *)matrix_D);
            __free_ddr((void *)matrix_P);
            __free_ddr((void *)matrix_invP);
        } /* while(j<num_run) */

    }  /* for(i=0, n=n_min; i<num_test; i++, n+=n_inc) */

    print_performance("dgeev", matrix_sizes, time_ev_all_runs, num_test, num_run);
    print_performance("zgetrf and zgetri", matrix_sizes, time_cinv_all_runs, num_test, num_run);
    
    printf("Passed.\n");
    
    exit( 0 );
} /* End of DGEEV Example */

/* Auxiliary routine: printing eigenvalues */
void print_eigenvalues( char* desc, int n, double* vector_r, double* vector_i ) 
{
   int j;
   
   if (!print_data)
     return;
   
   printf( "\n %s\n", desc );
   
   for( j = 0; j < n; j++ ) {
      if( vector_i[j] == (double)0.0 ) {
         printf( " %10.5f", vector_r[j] );
      } else {
         printf( " (%10.5f,%10.5f)", vector_r[j], vector_i[j] );
      }
   }
   printf( "\n" );
}

/* Auxiliary routine: printing eigenvectors */
void print_eigenvectors( char* desc, int n, double* vector_i, double* v, int ldv ) 
{
   int i, j;

   if (!print_data)
     return;
   
   printf( "\n %s\n", desc );
   
   for( i = 0; i < n; i++ ) {
      j = 0;
      while( j < n ) {
         if( vector_i[j] == (double)0.0 ) {
            printf( " %10.5f", v[i+j*ldv] );
            j++;
         } else {
            printf( " (%10.5f,%10.5f)", v[i+j*ldv],  v[i+(j+1)*ldv] );
            printf( " (%10.5f,%10.5f)", v[i+j*ldv], -v[i+(j+1)*ldv] );
            j += 2;
         }
      }
      printf( "\n" );
   }
}

void form_eigen_matrix(doublecomplex *mat, int n, double* vector_i, double* v, int ldv) 
{
    int i, j;
    
    for( i = 0; i < n; i++ ) {
      j = 0;
      while( j < n ) {
         if( vector_i[j] == (double)0.0 ) {
            mat[i+j*ldv].r = v[i+j*ldv];
            mat[i+j*ldv].i = 0;
            j++;
         } else {
            mat[i+j*ldv].r = v[i+j*ldv];
            mat[i+j*ldv].i = v[i+(j+1)*ldv];
            mat[i+(j+1)*ldv].r = v[i+j*ldv];
            mat[i+(j+1)*ldv].i = -v[i+(j+1)*ldv];
            j += 2;
         }
      }
   }
}

void form_diag_matrix(doublecomplex *matrix_diag, doublereal *vector_r, doublereal *vector_i, int n)
{
   int i, j;
  
   for(i=0; i<n; i++) {
      for(j=0; j<n; j++) {
	    matrix_diag[i+j*n].r = 0.;
	    matrix_diag[i+j*n].i = 0.;
      }
   }
   
   for(i=0; i<n; i++) {
      matrix_diag[i+i*n].r = vector_r[i];
      matrix_diag[i+i*n].i = vector_i[i];
   }   
}

void print_real_matrix(char* desc, double *mat, int m, int n, int ldv ) 
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

void print_complex_matrix(char* desc, doublecomplex *mat, int m, int n, int ldv ) 
{
   int i, j;
   double real, imag;

   if (!print_data)
     return;
   
   printf( "\n %s\n", desc );
   for(i=0; i<m; i++) {
      for(j=0; j<n; j++) {
         real = mat[i+j*ldv].r;
         imag = mat[i+j*ldv].i;
         if(imag >= 0) {
            printf( " %10.5f + %10.5fi", real, imag);
         }
         else {
            printf( " %10.5f - %10.5fi", real, fabs(imag));
         }
      }
      printf( "\n" );
  }
}

doublereal compute_evd_error(doublecomplex *matrix_reconstruct, doublereal *matrix_original, int n, int ld)
{
   int i, j;
   doublereal err_real, err_imag;
   doublereal abs_err;
   doublereal max_abs_err = 0.;

   for(i=0; i<n; i++) {
      for(j=0; j<n; j++) {
	    err_real = matrix_reconstruct[i+j*ld].r - matrix_original[i+j*ld];
	    err_imag = matrix_reconstruct[i+j*ld].i;
        
        abs_err = sqrt(err_real*err_real + err_imag*err_imag);
        
        if(abs_err > max_abs_err) {
          max_abs_err = abs_err;
        }
      }
   }
   
   return max_abs_err;
}

void print_performance(char * func_name, int *matrix_sizes, float *time, int num_test, int num_run)
{
    int i, j;
    float inst, min, max;
    float tot, avg;
    
    printf("Time (second) of %s for all matrix sizes:\n", func_name);
    printf("Matrix size   ");
    printf("    Min           Max        Average  ");
    printf("\n");
    
    for(i=0; i<num_test; i++)
    {
        printf("%11d ", matrix_sizes[i]);
        tot = 0.;
        max = 0.;
        min = 1e100;
        for(j=0; j<num_run; j++) 
          {
            inst = time[i*num_run+j];            
            tot += (double)inst;
            
            if(inst>max) max = inst;
            if(inst<min) min = inst;
          }
        avg = tot/(double)num_run;
        printf("%12f%12f%12f\n", min, max, avg);
    }
}

