/*
 * Copyright (c) 2015 - 2016, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/** ============================================================================
 *  @file       osal.h
 *
 *  @brief      OS Abstraction Layer header
 *
 *  ============================================================================
 */

#ifndef ti_OSAL__include
#define ti_OSAL__include

#ifdef __cplusplus
extern "C" {
#endif


#include <ti/osal/HwiP.h>
#include <ti/osal/MuxIntcP.h>
#include <ti/osal/SemaphoreP.h>
#include <ti/osal/CacheP.h>
#include <ti/osal/TimerP.h>

#define osal_OK        (0)
#define osal_FAILURE  (-(1))


typedef enum Osal_ThreadType_e {
    Osal_ThreadType_Hwi,   /*!< Current thread is a Hwi  */
    Osal_ThreadType_Swi,   /*!< Current thread is a Swi  */
    Osal_ThreadType_Task,  /*!< Current thread is a Task */
    Osal_ThreadType_Main   /*!< Current thread is Main   */
} Osal_ThreadType;

#ifdef __KLOCWORK__

extern void abort(void);
/* tell klockwork that OSAL_Assert(1) is fatal */
#define OSAL_Assert(x) do { if ((x)) {  abort();  } } while (0)

#else

/* Internal function for assert */
extern void _DebugP_assert(int32_t expression, const char *file, int32_t line);
/*!
 *  @brief  Assert checking function
 *
 *  If the expression is evaluated to true, the API does nothing.
 *  If it is evaluated to false, the underlying RTOS port implementation
 *  handles the assert via its mechanisms.
 *
 *  @param  expression Expression to evaluate
 */
/* in real code use TI's version of OSAL_Assert (which is also fatal for OSAL_Assert(1)*/
#define OSAL_Assert(expression) (_DebugP_assert((expression),      \
                                                  __FILE__, __LINE__))
#endif

/*!
 *  @brief  Function to get the current thread type.
 *
 *  @param  none
 *
 *  @return current thread type
 */
extern Osal_ThreadType Osal_getThreadType(void);

#ifdef __cplusplus
}
#endif

#endif /* ti_OSAL__include */
