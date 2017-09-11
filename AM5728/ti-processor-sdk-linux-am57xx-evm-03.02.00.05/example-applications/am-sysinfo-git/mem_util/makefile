#   ----------------------------------------------------------------------------
#  @file   Makefile
#
#  @path   
#
#  @desc   Makefile for mem_util (Debug and Release versions)
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
-include ../../../Rules.make

#   ----------------------------------------------------------------------------
#   Variables passed in externally
#   ----------------------------------------------------------------------------
CFLAGS ?= -march=armv5te -mtune=arm926ej-s
CROSS_COMPILE ?=

#   ----------------------------------------------------------------------------
#   Name of the Linux compiler
#   ----------------------------------------------------------------------------
CC := $(CROSS_COMPILE)gcc

#   ----------------------------------------------------------------------------
#   General options, sources and libraries
#   ----------------------------------------------------------------------------
NAME := mem_util
SRCS := mem_util.c
HDRS := 
OBJS :=
DEBUG :=
BIN := mem_util

#   ----------------------------------------------------------------------------
#   Compiler and Linker flags for Debug
#   ----------------------------------------------------------------------------
OBJDIR_D := Debug
BINDIR_D := $(OBJDIR_D)
LIBS_D := $(LIBS)
OBJS_D := $(SRCS:%.c=$(OBJDIR_D)/%.o)
ALL_DEBUG := -g3 -gdwarf-2

#   ----------------------------------------------------------------------------
#   Compiler and Linker flags for Release
#   ----------------------------------------------------------------------------
OBJDIR_R := Release
BINDIR_R := $(OBJDIR_R)
LIBS_R := $(LIBS)
OBJS_R := $(SRCS:%.c=$(OBJDIR_R)/%.o)
ALL_RELEASE :=

#   ----------------------------------------------------------------------------
#   Compiler include directories 
#   ----------------------------------------------------------------------------
INCLUDES := 

#   ----------------------------------------------------------------------------
#   All compiler options to be passed to the command line
#   ----------------------------------------------------------------------------
ALL_CFLAGS := $(INCLUDES)                   \
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
.PHONY: all
all: debug release

#   ----------------------------------------------------------------------------
#   Building Debug... 
#   ----------------------------------------------------------------------------
.PHONY: debug
debug: $(BINDIR_D)/$(BIN)

$(BINDIR_D)/$(BIN): $(OBJS_D)
	@echo Compiling Debug...
	$(CC) -o $@ $(OBJS_D) $(LIBS_D) $(LDFLAGS)  -Wl,-Map,$(BINDIR_D)/$(NAME).map

$(OBJDIR_D)/%.o : %.c $(HDRS)
	@mkdir -p $(OBJDIR_D)
	$(CC) $(ALL_DEBUG) $(DEFS) $(ALL_CFLAGS) -o$@ $<

#   ----------------------------------------------------------------------------
#   Building Release... 
#   ----------------------------------------------------------------------------
.PHONY: release
release: $(BINDIR_R)/$(BIN)

$(BINDIR_R)/$(BIN): $(OBJS_R)
	@echo Compiling Release...
	$(CC) -o $@ $(OBJS_R) $(LIBS_R) $(LDFLAGS) -Wl,-Map,$(BINDIR_R)/$(NAME).map


$(OBJDIR_R)/%.o : %.c $(HDRS)
	@mkdir -p $(OBJDIR_R)
	$(CC) $(DEFS) $(ALL_RELEASE) $(ALL_CFLAGS) -o$@ $<

.PHONY: clean
clean:
	@rm -rf $(OBJDIR_D)
	@rm -rf $(OBJDIR_R)

.PHONY: install
install:
	@if [ ! -d $(DESTDIR) ] ; then \
		echo "The extracted target filesystem directory doesn't exist."; \
		echo "Please run setup.sh in the SDK's root directory and then try again."; \
		exit 1; \
	fi
	@install -d $(DESTDIR)/usr/bin
	@if [ -e Release/mem_util ] ; then \
		install Release/mem_util $(DESTDIR)/usr/bin/mem_util ; \
		echo "mem_util release version installed."; \
	else \
		echo "mem_util release version not built - nothing to install!"; \
	fi

.PHONY: install_debug
install_debug:
	@if [ ! -d $(DESTDIR) ] ; then \
		echo "The extracted target filesystem directory doesn't exist."; \
		echo "Please run setup.sh in the SDK's root directory and then try again."; \
		exit 1; \
	fi
	@install -d $(DESTDIR)/usr/bin
	@if [ -e Debug/mem_util ] ; then \
		install Debug/mem_util $(DESTDIR)/usr/bin/mem_util ; \
		echo "mem_util debug version installed."; \
	else \
		echo "mem_util debug version not built - nothing to install!"; \
	fi
