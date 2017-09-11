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
void vadd_sub_section(float *a, float *b, float *c, int size)
{
    int start = 0;
    #pragma omp target data map(to:a[start:size], b[start:size]) map(tofrom:c[start:size])
    {
        /* At this point a,b have been copied to the device and space for c has also 
         * been allocated on the device */
        int i;
        int chunk_size = size/2;
        /* Operate on half of Array's a and b to generate half of c 
         * At this point, sub-sections are created out of the parent array sections a,b,c */
        #pragma omp target map(to:a[start:chunk_size], b[start:chunk_size]) \
                           map(tofrom:c[start:chunk_size])
        {
            #pragma omp parallel for private(i)
            for (i = start; i < start+chunk_size; i++)
                c[i] = a[i] + b[i];
        }
        
        /* Operate on the other half, simply by incrementing the start offset */
        start = chunk_size;
        #pragma omp target map(to:a[start:chunk_size], b[start:chunk_size]) \
                           map(tofrom:c[start:chunk_size])
        {
            #pragma omp parallel for private(i)
            for (i = start; i < start+chunk_size; i++)
                c[i] = a[i] + b[i];
        }
    }
}
