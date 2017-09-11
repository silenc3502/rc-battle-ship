#
# File Name: Makefile
#
# Description: Makefile to build kernel hplib utility module.
#
# Copyright (C) 2014 Texas Instruments, Incorporated
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the
# Free Software Foundation version 2.
#
# This program is distributed "as is" WITHOUT ANY WARRANTY of any kind,
# whether express or implied; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
#

obj-m := debugss_kmodule.o
CFLAGS_debugss_kmodule.o := -D$(PLATFORM)

KVERSION ?= $(shell uname -r)
KERNEL_SRC  ?= /lib/modules/$(KVERSION)/build

default all:
	make -C $(KERNEL_SRC) M=$(PWD) modules 

clean:
	make -C $(KERNEL_SRC) M=$(PWD) clean

modules_install: 
	make -C $(KERNEL_SRC) M=$(PWD) modules_install

install_ipk: default all
	install -m 644 -d $(DESTDIR)/lib/modules/$(KVERSION)/extra
	install -m 644 -d $(DESTDIR)/lib/udev/rules.d
	sudo mv debugss_kmodule.ko $(DESTDIR)/lib/modules/$(KVERSION)/extra 
	sudo cp ../debian/debugss-mod-dkms.udev $(DESTDIR)/lib/udev/rules.d/debugss-mod.rules

