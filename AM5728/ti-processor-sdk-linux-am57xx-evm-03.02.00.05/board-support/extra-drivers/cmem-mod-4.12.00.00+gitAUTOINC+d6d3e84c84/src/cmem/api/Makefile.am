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
## ======== src/cmem/api/Makefile.am ========
##

# additional include paths necessary to compile the library
AM_CFLAGS = -D_FILE_OFFSET_BITS=64 -I$(top_srcdir)/include -Wall @AM_CFLAGS@

###############################################################################
# THE LIBRARIES TO BUILD
###############################################################################

# the library names to build (note we are building shared libs)
lib_LTLIBRARIES = libticmem.la

# where to install the headers on the system
libticmem_ladir = $(includedir)/ti

# the list of header files that belong to the library (to be installed later)
libticmem_la_HEADERS = $(top_srcdir)/include/ti/cmem.h

# the sources to add to the library and to add to the source distribution
libticmem_la_SOURCES =  \
                        $(libticmem_la_HEADERS) \
                        cmem.c

# Add version info to the shared library
libticmem_la_LDFLAGS = -version-info 1:0:0
						
###############################################################################
