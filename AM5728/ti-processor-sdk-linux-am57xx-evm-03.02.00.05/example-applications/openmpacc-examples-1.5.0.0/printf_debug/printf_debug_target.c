/******************************************************************************
 * Copyright (c) 2013-2014, Texas Instruments Incorporated - http://www.ti.com/
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
#include <omp.h>
extern int printf(const char *_format, ...);
#pragma omp end declare target

void omp_runtime_functions_with_printf(int num_procs_to_set)
{
    #pragma omp target map(to:num_procs_to_set)
    {
        printf("Number of Processor Cores: %d\n", 
                omp_get_num_procs()
              );
        printf("Maximum number of OpenMP threads: %d\n", 
                omp_get_max_threads()
              );
        printf("Starting 1st parallel region..\n");

        #pragma omp parallel
        {
            #pragma omp master
            {
                printf("\tNumber of OpenMP threads in team: %d\n", 
                        omp_get_num_threads()
                      );
            }

            printf("\tHello from thread: %d\n", 
                    omp_get_thread_num()
                  );
        }

        printf("Setting number of OpenMP threads to use to %d\n", 
                num_procs_to_set
              );
        omp_set_num_threads(num_procs_to_set);
        printf("Starting 2nd parallel region..\n");

        #pragma omp parallel
        {
            #pragma omp master
            {
                printf("\tNumber of OpenMP threads in team: %d\n", 
                        omp_get_num_threads()
                      );
            }
            printf("\tHello from thread: %d\n", 
                    omp_get_thread_num()
                  );
        }
    }
}
