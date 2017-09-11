#   ----------------------------------------------------------------------------
#  @file   Makefile
#
#  @path   
#
#  @desc   Makefile for oprofile_example (Debug version)
#
#  @ver    1.10
#   ----------------------------------------------------------------------------
#  Copyright (c) Texas Instruments Incorporated 2002-2009
#
#  Use of this software is controlled by the terms and conditions found in the
#  license agreement under which this software has been supplied or provided.
#

#   ----------------------------------------------------------------------------
#   Included defined variables
#   ----------------------------------------------------------------------------
include ../../Rules.make

#   ----------------------------------------------------------------------------
#   Variables passed in externally
#   ----------------------------------------------------------------------------
PLATFORM ?=
ARCH ?=
CROSS_COMPILE ?=

#   ----------------------------------------------------------------------------
#   Name of the Linux compiler
#   ----------------------------------------------------------------------------
CC := $(CROSS_COMPILE)gcc

#   ----------------------------------------------------------------------------
#   General options, sources and libraries
#   ----------------------------------------------------------------------------
NAME := signal_parent
SRCS := signal_parent.c
HDRS := 
OBJS :=
DEBUG :=
BIN := signal_parent

#   ----------------------------------------------------------------------------
#   Compiler and Linker flags for Debug
#   ----------------------------------------------------------------------------
OBJDIR_D := Debug
BINDIR_D := $(OBJDIR_D)
LIBS_D := $(LIBS)
OBJS_D := $(SRCS:%.c=$(OBJDIR_D)/%.o)
ALL_DEBUG := -g3 -gdwarf-2

OBJDIR_D1 := Debug1
BINDIR_D1 := $(OBJDIR_D1)
OBJS_D1 := $(SRCS:%.c=$(OBJDIR_D1)/%.o)

#   ----------------------------------------------------------------------------
#   Compiler include directories 
#   ----------------------------------------------------------------------------
INCLUDES := 

#   ----------------------------------------------------------------------------
#   All compiler options to be passed to the command line
#   ----------------------------------------------------------------------------
ALL_CFLAGS := $(INCLUDES)                   \
              -static                       \
              -c                            \
              $(CFLAGS)

LDFLAGS :=

#   ----------------------------------------------------------------------------
#   Compiler symbol definitions 
#   ----------------------------------------------------------------------------
DEFS :=

#   ----------------------------------------------------------------------------
#   Compiler and Linker procedure
#   From this point and on changes are very unlikely.
#   ----------------------------------------------------------------------------
all: debug debug1

#   ----------------------------------------------------------------------------
#   Building Debug... 
#   ----------------------------------------------------------------------------
debug: $(BINDIR_D)/$(BIN)

$(BINDIR_D)/$(BIN): $(OBJS_D)
	@mkdir -p $(OBJDIR_D)
	@echo Compiling Debug...
	$(CC) -o $@ $(OBJS_D) $(LIBS_D) $(LDFLAGS)  -Wl,-Map,$(BINDIR_D)/$(NAME).map 

$(OBJDIR_D)/%.o : %.c $(HDRS)
	@mkdir -p $(OBJDIR_D)
	$(CC) $(ALL_DEBUG) $(DEFS) $(ALL_CFLAGS) -o$@ $<

#   ----------------------------------------------------------------------------
#   Building Debug... 
#   ----------------------------------------------------------------------------
debug1: $(BINDIR_D1)/$(BIN).opt

$(BINDIR_D1)/$(BIN).opt: $(OBJS_D1)
	@mkdir -p $(OBJDIR_D1)
	@echo Compiling Debug...
	$(CC) -o $@ $(OBJS_D1) $(LIBS_D) $(LDFLAGS)  -Wl,-Map,$(BINDIR_D1)/$(NAME).map 

$(OBJDIR_D1)/%.o : %.c $(HDRS)
	@mkdir -p $(OBJDIR_D1)
	$(CC) $(ALL_DEBUG) $(DEFS) -DUSE_SIGNALS $(ALL_CFLAGS) -o$@ $<

clean:
	@rm -rf $(OBJDIR_D)
	@rm -rf $(OBJDIR_D1)


install:
	@if [ ! -d $(DESTDIR) ] ; then \
		echo "The extracted target filesystem directory doesn't exist."; \
		echo "Please run setup.sh in the SDK's root directory and then try again."; \
		exit 1; \
	fi
	@install -d $(DESTDIR)/usr/bin
	@if [ -e $(OBJDIR_D1)/signal_parent.opt ] ; then \
		install $(OBJDIR_D1)/signal_parent.opt $(DESTDIR)/usr/bin/signal_parent.opt ; \
		echo "oprofile_example optimized debug version installed."; \
	else \
		echo "oprofile_example optimized debug version not built - nothing to install!"; \
	fi
	@if [ -e $(OBJDIR_D)/signal_parent ] ; then \
		install $(OBJDIR_D)/signal_parent $(DESTDIR)/usr/bin/signal_parent ; \
		echo "oprofile_example debug version installed."; \
	else \
		echo "oprofile_example debug version not built - nothing to install!"; \
	fi
