#*
#*
#* Copyright (C) 2016 Texas Instruments Incorporated - http://www.ti.com/
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
ifeq ($(DEVICE), am33xx)
DEFINES = -DPRUSS_V2\=1
else
ifeq ($(DEVICE), am57xx)
DEFINES = -DPRUSS_V2\=1
else
DEFINES = -DPRUSS_V2\=0
endif
endif

SOURCE=uio_test_pru.asm
LNKCMD=lnk.cmd

all: uio_test_pru0.bin uio_test_pru1.bin

uio_test_pru%.asm: $(SOURCE)
	cp $(SOURCE) $@

uio_test_pru0.obj: uio_test_pru0.asm
	$(CGT_PRU)/bin/clpru $< $(DEFINES) -DCORE_PRU\=0

uio_test_pru1.obj: uio_test_pru1.asm
	$(CGT_PRU)/bin/clpru $< $(DEFINES) -DCORE_PRU\=1

uio_test_pru%.out: uio_test_pru%.obj
	$(CGT_PRU)/bin/clpru $< \
	  -z $(LNKCMD) -e ARM_TO_PRU_INTERRUPT_POLL \
	  -o $@ -m $(subst .out,.map,$@)

uio_test_pru%.bin: uio_test_pru%.out
	objcopy -O binary -I elf32-little $< $@

clean:
	@rm -fr *.bin *.obj *.out *.map
