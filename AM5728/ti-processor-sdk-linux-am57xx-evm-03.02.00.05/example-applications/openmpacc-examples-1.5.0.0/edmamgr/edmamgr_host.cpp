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
extern "C"{
    #include "edmamgr.h" 
}

/* 
 * NOTE: 
 * The OpenMP 4.0 Specification provides a means to check a condition 
 * based on which a target region may be offloaded to an accelerator 
 * or run on the host. This functionality is provided by the 'if' 
 * clause used with #pragma omp target. In order for that scenario
 * to work these functions are meant to reflect a host version of 
 * the EdmaMgr functions used within the OpenMP target region. 
 * Implementation for these functions are not provided but they may be 
 * instantiated with a host side edma library call if required.  
 */
EdmaMgr_Handle  EdmaMgr_alloc            (int32_t max_linked_transfers){}

int32_t         EdmaMgr_free             (EdmaMgr_Handle h){}

void            EdmaMgr_wait             (EdmaMgr_Handle h){} 

int32_t         EdmaMgr_copy1D1D         (EdmaMgr_Handle  h, 
                                          void *__restrict__  src, 
                                          void *__restrict__  dst, 
                                          int32_t         num_bytes){}

int32_t         EdmaMgr_copy1D2D         (EdmaMgr_Handle  h, 
                                          void *__restrict__  src, 
                                          void *__restrict__  dst, 
                                          int32_t         num_bytes, 
                                          int32_t         num_lines, 
                                          int32_t         pitch){}

int32_t         EdmaMgr_copy2D1D         (EdmaMgr_Handle  h,
                                          void *__restrict__  src,
                                          void *__restrict__  dst,
                                          int32_t         num_bytes,
                                          int32_t         num_lines,
                                          int32_t         pitch){}

int32_t         EdmaMgr_copy2D2D         (EdmaMgr_Handle  h,
                                          void *__restrict__  src,
                                          void *__restrict__  dst,
                                          int32_t         num_bytes,
                                          int32_t         num_lines,
                                          int32_t         pitch){}

int32_t         EdmaMgr_copy2D2DSep      (EdmaMgr_Handle  h,
                                          void *__restrict__  src,
                                          void *__restrict__  dst,
                                          int32_t         num_bytes,
                                          int32_t         num_lines,
                                          int32_t         src_pitch,
                                          int32_t         dst_pitch){}

int32_t         EdmaMgr_copy1D1DLinked   (EdmaMgr_Handle  h,
                                          void *__restrict__  src[],
                                          void *__restrict__  dst[],
                                          int32_t         num_bytes[],
                                          int32_t         num_transfers){}

int32_t         EdmaMgr_copy1D2DLinked   (EdmaMgr_Handle  h,
                                          void *__restrict__  src[],
                                          void *__restrict__  dst[],
                                          int32_t         num_bytes[],
                                          int32_t         num_lines[],
                                          int32_t         pitch[],
                                          int32_t         num_transfers){}

int32_t         EdmaMgr_copy2D1DLinked   (EdmaMgr_Handle  h,
                                          void *__restrict__  src[],
                                          void *__restrict__  dst[],
                                          int32_t         num_bytes[],
                                          int32_t         num_lines[],
                                          int32_t         pitch[],
                                          int32_t         num_transfers){}

int32_t         EdmaMgr_copy2D2DLinked   (EdmaMgr_Handle  h,
                                          void *__restrict__  src[],
                                          void *__restrict__  dst[],
                                          int32_t         num_bytes[],
                                          int32_t         num_lines[],
                                          int32_t         pitch[],
                                          int32_t         num_transfers){}

int32_t         EdmaMgr_copy2D2DSepLinked(EdmaMgr_Handle  h,
                                          void *__restrict__  src[],
                                          void *__restrict__  dst[],
                                          int32_t         num_bytes[],
                                          int32_t         num_lines[],
                                          int32_t         src_pitch[],
                                          int32_t         dst_pitch[],
                                          int32_t         num_transfers){}

int32_t         EdmaMgr_copyFast         (EdmaMgr_Handle  h,
                                          void *__restrict__  src,
                                          void *__restrict__  dst){}

int32_t         EdmaMgr_copyLinkedFast   (EdmaMgr_Handle  h,
                                          void *__restrict__  src[],
                                          void *__restrict__  dst[],
                                          int32_t         num_transfers){}
