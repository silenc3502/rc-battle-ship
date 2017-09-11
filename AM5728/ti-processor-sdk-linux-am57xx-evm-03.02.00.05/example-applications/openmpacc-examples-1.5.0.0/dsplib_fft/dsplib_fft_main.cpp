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
#include <cstdlib>
#include <cmath>
#include <cstring>
#include <iostream>

using namespace std;

void tw_gen(float *w, int n);

extern "C"
{
    extern void dsplib_fft(int N, int bufsize, float* x, float* w, float *y, 
            int n_min, int offset, int n_max);
}

/* ======================================================================== */
/*  Initialized arrays with fixed test data.                                */
/* ======================================================================== */
#define  FFTSZ (64*1024)
#define  PAD   0

float x[2*FFTSZ];
float y[2*FFTSZ];
float w[2*FFTSZ];

/* ======================================================================== */
/*  MAIN                                                                    */
/* ======================================================================== */
int main ()
{
    int i, j;
    int rad = 4;

    /* ---------------------------------------------------------------- */
    /* Initialize input vector                                          */
    /* ---------------------------------------------------------------- */
    for (i = 0; i < FFTSZ; i++)
    {
        x[PAD + 2*i]     = sin (2 * 3.1415 * 50 * i / (double) FFTSZ);
        x[PAD + 2*i + 1] = 0;
    }

    memset (y, 0xA5, sizeof (y));       // fix value of output
    tw_gen (w, FFTSZ);               // Generate twiddle factors

    int bufsize = (2*FFTSZ + PAD + PAD);
    /* ---------------------------------------------------------------- */
    /* Call dsplib_fft target code                                      */
    /* ---------------------------------------------------------------- */
    dsplib_fft(FFTSZ, bufsize, x, w, y, rad, 0, FFTSZ);

    cout << "PASS!" << endl;
}

/******************************************************************************
* Function for generating Specialized sequence of twiddle factors
******************************************************************************/
void tw_gen(float *w, int n)
{
    int i, j, k;
    const double PI = 3.141592654;

    for (j = 1, k = 0; j <= n >> 2; j = j << 2)
    {
        for (i = 0; i < n >> 2; i += j)
        {
            w[k]     = (float) sin (2 * PI * i / n);
            w[k + 1] = (float) cos (2 * PI * i / n);
            w[k + 2] = (float) sin (4 * PI * i / n);
            w[k + 3] = (float) cos (4 * PI * i / n);
            w[k + 4] = (float) sin (6 * PI * i / n);
            w[k + 5] = (float) cos (6 * PI * i / n);
            k += 6;
        }
    }
}
