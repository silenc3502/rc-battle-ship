UNAME_M :=$(shell uname -m)
ifneq (,$(findstring 86, $(UNAME_M)))
    # In a cross compile environment we are assuming that the EVM file system
    # is located on the build host and necessary ARM libraries are installed
    # on that file system.
    ifneq ($(MAKECMDGOALS),clean)
       ifeq ($(TARGET_ROOTDIR),)
         $(error Environment variable TARGET_ROOTDIR must be defined. Set it to point at the EVM root file system)
       endif
    endif

    # gcc ARM cross compiler will not, by default, search the host's
    # /usr/include.  Explicitly specify here to find dependent vendor headers
    CC = arm-linux-gnueabihf-gcc
else
    CC = gcc 
endif

CFLAGS = -g -O2 -I$(TARGET_ROOTDIR)/usr/include -I$(LINALG_DIR)/packages/ti/linalg

LIB_DIR = $(LINALG_DIR)/packages/ti/linalg/lib/
LD_FLAGS=-L$(TARGET_ROOTDIR)/lib -L$(TARGET_ROOTDIR)/usr/lib -Wl,-rpath-link,$(TARGET_ROOTDIR)/lib -Wl,-rpath-link,$(TARGET_ROOTDIR)/usr/lib
BLASLIB = $(LIB_DIR)libcblas_armplusdsp.a -lOpenCL -locl_util -lstdc++ -lrt -lm -lgomp -lpthread -ldl
LAPACKLIB = $(LIB_DIR)libcblaswr.a $(LIB_DIR)liblapack.a $(LIB_DIR)libf2c.a $(LIB_DIR)libcblas_armplusdsp.a -lOpenCL -locl_util -lstdc++ -lrt -lm -lgomp  -lpthread -ldl

%.o: %.c
	$(CC) -c $(CFLAGS) $<
	echo Compiling $<

$(EXE):

clean::
	@rm -f $(EXE) *.o *.dat

