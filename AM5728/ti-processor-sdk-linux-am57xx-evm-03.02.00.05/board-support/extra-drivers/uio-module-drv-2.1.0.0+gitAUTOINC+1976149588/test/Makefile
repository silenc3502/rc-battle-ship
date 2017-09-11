#*
#*
#* Copyright (C) 2013-2016 Texas Instruments Incorporated - http://www.ti.com/
#*
#*
#*  Redistribution and use in source and binary forms, with or without
#*  modification, are permitted provided that the following conditions
#*  are met:
#*
#*    Redistributions of source code must retain the above copyright
#*    notice, this list of conditions and the following disclaimer.
#*
#*    Redistributions in binary form must reproduce the above copyright
#*    notice, this list of conditions and the following disclaimer in the 
#*    documentation and/or other materials provided with the
#*    distribution.
#*
#*    Neither the name of Texas Instruments Incorporated nor the names of
#*    its contributors may be used to endorse or promote products derived
#*    from this software without specific prior written permission.
#*
#*
#*
#*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
#*  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
#*  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS OR
#*  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
#*  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
#*  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
#*  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
#*  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
#*  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
#*  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
#*  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#*
ifeq ($(BUILD_LOCAL),true)
CC = $(CROSS_COMPILE)gcc
LD = $(CROSS_COMPILE)ld
endif

# Compiler name (with path, if not in path)
LDFLAGS+= -lrt -lpthread
#LIBS=${UIO_LIB_DIR}/usr/lib/libuio.a

# Mainly used for include paths
INCLUDES=-I../

DEFINES=-D__ARMv7

SOURCES=uio_cic2_int_multithread_test.c
SOURCES3=uio_mem_test.c
SOURCES4=uio_int_test.c

OBJECTS=$(patsubst %.c,%.o, $(SOURCES))
OBJECTS3=$(patsubst %.c,%.o, $(SOURCES3))
OBJECTS4=$(patsubst %.c,%.o, $(SOURCES4))

EXECUTABLE1=uio_cic2_int_multithread_test
EXECUTABLE2=prussdrv_test
EXECUTABLE3=uio_mem_test
EXECUTABLE4=uio_int_test

.PHONY: $(EXECUTABLE1) $(EXECUTABLE2) $(EXECUTABLE3) $(EXECUTABLE4)

ifeq ($(DEVICE), keystone)
all: $(EXECUTABLE1) $(EXECUTABLE3) $(EXECUTABLE4)
endif
ifeq ($(DEVICE), am33xx)
all: $(EXECUTABLE2)  $(EXECUTABLE3) $(EXECUTABLE4)
endif
ifeq ($(DEVICE), am57xx)
all: $(EXECUTABLE2)  $(EXECUTABLE3) $(EXECUTABLE4)
endif

$(EXECUTABLE1): $(OBJECTS) $(LIBS)
	@mkdir -p $(dir $@)
	$(CC)  $(OBJECTS) $(LIBS) -o $@ $(LDFLAGS)

$(EXECUTABLE3): $(OBJECTS3) $(LIBS)
	@mkdir -p $(dir $@)
	$(CC)  $(OBJECTS3) $(LIBS) -o $@ $(LDFLAGS)

$(EXECUTABLE4): $(OBJECTS4) $(LIBS)
	@mkdir -p $(dir $@)
	$(CC)  $(OBJECTS4) $(LIBS) -o $@ $(LDFLAGS)

%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(INCLUDES) $(DEFINES) -c $< -o $@

$(EXECUTABLE2):
	@make -C $(EXECUTABLE2)

clean:
	@rm -fr *.o $(EXECUTABLE1) $(EXECUTABLE3) $(EXECUTABLE4)
	@rm -fr $(OBJECTS)
	@make -C $(EXECUTABLE2) clean
