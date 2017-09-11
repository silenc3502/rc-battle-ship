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
#include "edmamgr.h"
#include "omp.h"

float dsp_speed()
{
    const unsigned DSP_PLL  = 122880000;
    char *BOOTCFG_BASE_ADDR = (char*)0x02620000;
    char *CLOCK_BASE_ADDR   = (char*)0x02310000;
    int MAINPLLCTL0         = (*(int*)(BOOTCFG_BASE_ADDR + 0x350));
    int MULT                = (*(int*)(CLOCK_BASE_ADDR + 0x110));
    int OUTDIV              = (*(int*)(CLOCK_BASE_ADDR + 0x108));

    unsigned mult = 1 + ((MULT & 0x3F) | ((MAINPLLCTL0 & 0x7F000) >> 6));
    unsigned prediv = 1 + (MAINPLLCTL0 & 0x3F);
    unsigned output_div = 1 + ((OUTDIV >> 19) & 0xF);
    unsigned speed = DSP_PLL * mult / prediv / output_div;
    return speed / 1e6;
}

float bandwidth(char *dst, char *src, int size)
{
    EdmaMgr_Handle chan = EdmaMgr_alloc(1);

    int64_t t0 = __clock64();
    EdmaMgr_copy1D1D(chan, src, dst, size);
    EdmaMgr_wait(chan);
    int64_t t1 = __clock64();

    EdmaMgr_free(chan);

    /*-------------------------------------------------------------------------
    * return GB/s
    *------------------------------------------------------------------------*/
    return ((float)size/1e9) / ((t1-t0)/(dsp_speed()*1e6));
}
