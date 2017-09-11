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
include ../make.inc

OA_TC_OPTS 			= -O3
ifeq ($(VERBOSE),1)
OA_HC_OPTS 			= -O3 -Wall -Wextra -fopenmp
OA_SHELL_OPTS 		= -v -k --hc="$(OA_HC_OPTS)" --tc="$(OA_TC_OPTS)"
else
OA_HC_OPTS 			= -O3 -fopenmp
OA_SHELL_OPTS 		= --hc="$(OA_HC_OPTS)" --tc="$(OA_TC_OPTS)"
endif
CPP_OPTS 			= -O3

EXE     	= target_orphan_call
HOST_CODE 	= torphan_call_main.cpp
TARGET_CODE = torphan_call_target.c compute.c
OBJS 		= $(patsubst %.cpp,%.o,$(HOST_CODE))

$(EXE): $(OBJS)
	$(OA_SHELL) $(OA_SHELL_OPTS) $(OBJS) $(TARGET_CODE) -o $@

%.o: %.cpp
	$(CPP) $(CPP_OPTS) -c $<

clean:
	@rm -f $(EXE) $(OA_SHELL_TMP_FILES) *.log
