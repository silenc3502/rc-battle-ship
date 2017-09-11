##
##  Copyright (c) 2013, Texas Instruments Incorporated
##
##  Redistribution and use in source and binary forms, with or without
##  modification, are permitted provided that the following conditions
##  are met:
##
##  *  Redistributions of source code must retain the above copyright
##     notice, this list of conditions and the following disclaimer.
##
##  *  Redistributions in binary form must reproduce the above copyright
##     notice, this list of conditions and the following disclaimer in the
##     documentation and/or other materials provided with the distribution.
##
##  *  Neither the name of Texas Instruments Incorporated nor the names of
##     its contributors may be used to endorse or promote products derived
##     from this software without specific prior written permission.
##
##  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
##  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
##  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
##  PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
##  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
##  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
##  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
##  OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
##  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
##  OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
##  EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
##
## ======== src/test/Makefile.am ========
##

# additional include paths necessary to compile the program
AM_CFLAGS = -D_FILE_OFFSET_BITS=64 -I$(top_srcdir)/include -Wall @AM_CFLAGS@ 

###############################################################################
# THE PROGRAMS TO BUILD
###############################################################################

# the program to build (the names of the final binaries)
bin_PROGRAMS = apitest multi_process translate

common_sources = $(top_srcdir)/include/ti/cmem.h

# list of sources for the 'apitest' binary
apitest_SOURCES = $(common_sources) apitest.c

# list of sources for the 'multi_process' binary
multi_process_SOURCES = $(common_sources) multi_process.c

# list of sources for the 'translate' binary
translate_SOURCES = $(common_sources) translate.c

common_libraries = $(top_builddir)/src/cmem/api/libticmem.la

# the additional libraries needed to link 'apitest'
apitest_LDADD = $(common_libraries) \
                $(AM_LDFLAGS)

# the additional libraries needed to link 'multi_process'
multi_process_LDADD = $(common_libraries) \
                $(AM_LDFLAGS)

# the additional libraries needed to link 'translate'
translate_LDADD = $(common_libraries) \
                $(AM_LDFLAGS)

###############################################################################

