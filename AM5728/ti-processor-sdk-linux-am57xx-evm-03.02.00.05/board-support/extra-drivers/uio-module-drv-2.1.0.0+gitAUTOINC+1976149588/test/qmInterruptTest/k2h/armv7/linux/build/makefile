#
# Macro definitions referenced below
#

empty =
space =$(empty) $(empty)

ARMV7OBJDIR ?= ./obj
ARMV7BINDIR ?= ./bin
ARMV7LIBDIR ?= ./lib

ARMV7OBJDIR := $(ARMV7OBJDIR)/qmss/example/qmInterruptTest
ARMV7BINDIR := $(ARMV7BINDIR)/qmss/example

#Cross tools
ifdef CROSS_TOOL_INSTALL_PATH
# Support backwards compatibility with KeyStone1 approach
 CC = $(CROSS_TOOL_INSTALL_PATH)/$(CROSS_TOOL_PRFX)gcc
 AC = $(CROSS_TOOL_INSTALL_PATH)/$(CROSS_TOOL_PRFX)as
 AR = $(CROSS_TOOL_INSTALL_PATH)/$(CROSS_TOOL_PRFX)ar
 LD = $(CROSS_TOOL_INSTALL_PATH)/$(CROSS_TOOL_PRFX)gcc
endif

# INCLUDE Directories
QMSS_INC_DIR = $(PDK_INSTALL_PATH)/ti/drv/qmss
CPPI_INC_DIR = $(PDK_INSTALL_PATH)/ti/drv/cppi

QMSS_SRC_DIR ?= $(PDK_INSTALL_PATH)/ti/drv/qmss
QMSS_SAMPLE_DIR = $(QMSS_SRC_DIR)/example/qmInterruptTest
QMSS_ARM_LIN_SAMPLE_DIR = $(QMSS_SAMPLE_DIR)/$(DEVICE)/armv7/linux

INCDIR := $(PDK_INSTALL_PATH);$(CPPI_INC_DIR);$(QMSS_INC_DIR);$(QMSS_SAMPLE_DIR)/src;$(QMSS_ARM_LIN_SAMPLE_DIR)

# Libraries
QMSS_LIB = -lqmss
CPPI_LIB = -lcppi
RM_LIB = -lrm

ifeq ($(USEDYNAMIC_LIB), yes)
#presuming ARM executable would depend on dynamic library dependency
EXE_EXTN = _so
LIBS     = $(QMSS_LIB) $(CPPI_LIB) $(RM_LIB)
else
#forcing ARM executable to depend on static LLD libraries
EXE_EXTN =
LIBS     = -static $(QMSS_LIB) $(CPPI_LIB) $(RM_LIB) -Wl,-Bdynamic
endif

# Compiler options
INTERNALDEFS = $(DEBUG_FLAG) -D__ARMv7 -DDEVICE_K2H -D_VIRTUAL_ADDR_SUPPORT -D__LINUX_USER_SPACE -D_LITTLE_ENDIAN=1 -DMAKEFILE_BUILD -DINTERNAL_LINKING_RAM

# Linker options
INTERNALLINKDEFS = -Wl,--start-group -L $(ARMV7LIBDIR) $(LIBS) -lrt -Wl,--end-group -pthread $(LDFLAGS)

EXE=qmInterruptTest$(EXE_EXTN).out
OBJEXT = o 
SRCDIR = $(QMSS_SAMPLE_DIR)/src:$(QMSS_ARM_LIN_SAMPLE_DIR):

INCS = -I. -I$(strip $(subst ;, -I,$(INCDIR)))

VPATH=$(SRCDIR)

#List the Source Files
TEST_SRC = \
    infrastructure_multicoremode.c \
    fw_main.c \
    fw_init.c \
    fw_osal.c \
    fw_mem_allocator.c \
    sockutils.c

# FLAGS for the SourceFiles
SRC_CFLAGS = -I. $(CFLAGS)

# Make Rule for the SRC Files
TEST_SRC_OBJS = $(patsubst %.c, $(ARMV7OBJDIR)/%.$(OBJEXT), $(TEST_SRC))

all:$(ARMV7BINDIR)/$(EXE)

$(ARMV7BINDIR)/$(EXE): $(TEST_SRC_OBJS) $(ARMV7BINDIR)/.created $(ARMV7OBJDIR)/.created
	@echo linking $(TEST_SRC_OBJS) into $@ ...
	@echo linking options $(CC) $(INTERNALLINKDEFS)
	@$(CC) $(TEST_SRC_OBJS) $(INTERNALLINKDEFS) -o $@

$(ARMV7OBJDIR)/%.$(OBJEXT): %.c $(ARMV7OBJDIR)/.created
	@echo compiling $< ...
	@$(CC) -c $(SRC_CFLAGS) $(INTERNALDEFS) $(INCS)  $< -o $@

$(ARMV7OBJDIR)/.created:
	@mkdir -p $(ARMV7OBJDIR)
	@touch $(ARMV7OBJDIR)/.created

$(ARMV7BINDIR)/.created:
	@mkdir -p $(ARMV7BINDIR)
	@touch $(ARMV7BINDIR)/.created

clean:
	@rm -fr $(ARMV7OBJDIR)
	@rm -fr $(ARMV7BINDIR)

