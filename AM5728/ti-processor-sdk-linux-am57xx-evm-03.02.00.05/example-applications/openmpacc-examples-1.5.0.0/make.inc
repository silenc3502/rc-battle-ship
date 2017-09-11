#*****************************************************************************
# Copyright (c) 2014, Texas Instruments Incorporated - http://www.ti.com/
#   All rights reserved.
#
#   Redistribution and use in source and binary forms, with or without
#   modification, are permitted provided that the following conditions are met:
#       * Redistributions of source code must retain the above copyright
#         notice, this list of conditions and the following disclaimer.
#       * Redistributions in binary form must reproduce the above copyright
#         notice, this list of conditions and the following disclaimer in the
#         documentation and/or other materials provided with the distribution.
#       * Neither the name of Texas Instruments Incorporated nor the
#         names of its contributors may be used to endorse or promote products
#         derived from this software without specific prior written permission.
#
#   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
#   AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
#   IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE  
#   ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
#   LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
#   CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
#   SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
#   INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
#   CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
#   ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
#   THE POSSIBILITY OF SUCH DAMAGE.
#****************************************************************************
CL                    = cl6x
OA_SHELL 	          = clacc
CPP 		          = arm-linux-gnueabihf-g++
CC  		          = arm-linux-gnueabihf-gcc
OA_SHELL_TMP_FILES    = *.out __TI_CLACC_KERNEL.c *.cl *.asm *.dsp_h *.bc *.objc *.if *.map *.opt *.int.c *.o *.obj __s2s_predef.gcc_h

# temporary workaround for locale bug
LC_ALL?=C
export LC_ALL

UNAME_M :=$(shell uname -m)
ifneq (,$(findstring x86, $(UNAME_M)))
 ifeq ($(TARGET_ROOTDIR),)
 $(call error,ERROR - TARGET_ROOTDIR must be set for cross compiling)
 endif
endif

ifeq ($(TI_OCL_CGT_INSTALL),)
  TI_OCL_CGT_INSTALL = $(TARGET_ROOTDIR)/usr/share/ti/cgt-c6x
endif
