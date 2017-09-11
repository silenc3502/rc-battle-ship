/* ======================================================================== *
 * IMGLIB -- TI Image and Video Processing Library                          *
 *                                                                          *
 *                                                                          *
 * Copyright (C) 2012 Texas Instruments Incorporated - http://www.ti.com/   *
 *                                                                          *
 *                                                                          *
 *  Redistribution and use in source and binary forms, with or without      *
 *  modification, are permitted provided that the following conditions      *
 *  are met:                                                                *
 *                                                                          *
 *    Redistributions of source code must retain the above copyright        *
 *    notice, this list of conditions and the following disclaimer.         *
 *                                                                          *
 *    Redistributions in binary form must reproduce the above copyright     *
 *    notice, this list of conditions and the following disclaimer in the   *
 *    documentation and/or other materials provided with the                *
 *    distribution.                                                         *
 *                                                                          *
 *    Neither the name of Texas Instruments Incorporated nor the names of   *
 *    its contributors may be used to endorse or promote products derived   *
 *    from this software without specific prior written permission.         *
 *                                                                          *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS     *
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT       *
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR   *
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT    *
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,   *
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT        *
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,   *
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY   *
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT     *
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE   *
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.    *
 * ======================================================================== */

#ifndef IMG_PROFILE_H_
#define IMG_PROFILE_H_ 1

/* Memory utility includes */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "IMG_utility.h"

/* Profiling defines */
#define img_PROFILE_MAX_TESTS   12
#define img_PROFILE_MAX_ORDER    4

/* Profiling globals */
extern int     est_order;    // Order for the pseudo-inverse (2/3)
extern int     est_count;    // Number of tests used in estimating formula
extern int     est_test;     // Use current test in formula estimation
extern clock_t beg_count;    // Begin cycle count for profiling
extern clock_t end_count;    // End cycle count for profiling
extern clock_t overhead;     // Cycle profiling overhead
extern clock_t cycles[img_KERNEL_CNT];  // Cycle counts for C-natural and optimized

/* Profile formula calculation variables */
extern int   *AA[];
extern int   *bb;
extern float *xx;

/* Cycle formula estimation pseudo-inverse routine */
extern void IMG_pseudo_inverse (int **A, int *b, float *x, short m, short n);
extern void IMG_print_formula  (float *x);


/* Profiling initialization routine */
static inline void IMG_profile_init (int order, char *kernel_name)
{
  short i;

  /* Ensure the order is valid */
  IMG_assert (order <= img_PROFILE_MAX_ORDER, "Invalid cycle formula estimation order");

  /* Allocate A matrix */
  for (i=0; i<img_PROFILE_MAX_TESTS; i++) {
    AA[i] = (int *) malloc (order * sizeof(int));
  }

  /* Allocate b/x vectors */
  bb = (int *)   malloc (order * sizeof(int));
  xx = (float *) malloc (order * sizeof(float));

  /* Compute the overhead of calling clock twice to get timing info. */
  beg_count = clock();
  end_count = clock();
  overhead  = end_count - beg_count;

  /* Clear cycle counts */
  cycles[img_KERNEL_OPT] = (clock_t) 0;
  cycles[img_KERNEL_CN] = (clock_t) 0;

  /* Initialize profiling globals */
  est_order = order;
  est_count = 0;

  /* Informational message */
  printf("\n\nTesting: %s\n",kernel_name);
  printf("------------------------------------------------------------------------\n");
}


/* Profiling start routine */
static inline void IMG_profile_start (int kernel)
{
  /* Set the active kernel for subsequent calls */
  act_kernel = kernel;

  beg_count = clock();
}


/* Profiling stop (and calculate) routine */
static inline void IMG_profile_stop (void)
{
  end_count = clock();
  /* Calculate and store cycles */
  cycles[act_kernel] = end_count - beg_count - overhead;
}


/* Profiling add calculation */
static inline void IMG_formula_add_test (int Ax0, int Ax1, int Ax2)
{
  /* Display order-2 parameters and measurements always */
  printf("Test %2d    N: %5d,  ", test_index, Ax0);
  /* Order-3 parameter */
  if (est_order > 2) {
    printf("M: %5d,  ", Ax1);
  }
  /* Order-4 parameter */
  if (est_order > 3) {
    printf("L: %5d,  ", Ax2);
  }
  /* Display measurements always */
  printf("OPT: %6d,  CN: %6d", cycles[img_KERNEL_OPT], cycles[img_KERNEL_CN]);

  /* Add test case to matrix and vectors for cycle formula estimation */
  if (est_test) {
    /* Fill in all measurements (even if NULL) */
    AA[est_count][0] = Ax0;
    AA[est_count][1] = Ax1;
    AA[est_count][2] = Ax2;
    
    /* Provide a 1 for the last one (constant overhead) */
    AA[est_count][est_order-1] = 1;

    /* Add cycles to measurement vector */
    bb[est_count] = cycles[img_KERNEL_OPT];
    
    est_count += 1;
    printf("  (Cycle Estimation)");
  }

  printf("\n");
}


/* Estimate cycle formula using Moore-Penrose pseudo-inverse */
static inline void IMG_profile_cycle_formula (void)
{
  short i;

  /* We reached this point - provide pass indication */
  printf("------------------------------------------------------------------------\n");
  printf("Success: All %d tests pass\n", testcases);
  
  /* Solve overdetermined set of equations for x */
  IMG_pseudo_inverse (AA, bb, xx, est_count, est_order);

  /* Provide cycle formula */
  IMG_print_formula (xx);

  /* Free memory allocated in IMG_profile_init() */
  for (i=0; i<img_PROFILE_MAX_TESTS; i++) {
    free (AA[i]);
  }
  free (bb);
  free (xx);
}


/* Add information regarding variables */
static inline void IMG_profile_cycle_info (char *info)
{
  /* Print cycle formula descriptive information */
  printf ("Where:   %s\n", info);
}

#endif

/* =========================================================================*/
/*  End of file:  IMG_profile.h                                             */
/* =========================================================================*/
