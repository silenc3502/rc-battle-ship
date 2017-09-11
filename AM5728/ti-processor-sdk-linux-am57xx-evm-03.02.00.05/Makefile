-include Rules.make

MAKE_JOBS ?= 1

all: linux matrix-gui arm-benchmarks am-sysinfo oprofile-example matrix-gui-browser refresh-screen cryptodev debugss-module-drv gdbserverproxy-module-drv opencl-examples openmpacc-examples qt-opencv-opencl-opengl-multithreaded u-boot-spl omapconf linux-dtbs ti-sgx-ddk-km cmem-mod pru-icss dual-camera-demo image-gallery uio-module-drv ti-gc320-driver ti-ipc barcode-roi linalg-examples 
clean: linux_clean matrix-gui_clean arm-benchmarks_clean am-sysinfo_clean oprofile-example_clean matrix-gui-browser_clean refresh-screen_clean cryptodev_clean debugss-module-drv_clean gdbserverproxy-module-drv_clean opencl-examples_clean openmpacc-examples_clean qt-opencv-opencl-opengl-multithreaded_clean u-boot-spl_clean omapconf_clean linux-dtbs_clean ti-sgx-ddk-km_clean cmem-mod_clean pru-icss_clean dual-camera-demo_clean image-gallery_clean uio-module-drv_clean ti-gc320-driver_clean ti-ipc_clean barcode-roi_clean linalg-examples_clean 
install: linux_install matrix-gui_install arm-benchmarks_install am-sysinfo_install oprofile-example_install matrix-gui-browser_install refresh-screen_install cryptodev_install debugss-module-drv_install gdbserverproxy-module-drv_install opencl-examples_install openmpacc-examples_install qt-opencv-opencl-opengl-multithreaded_install u-boot-spl_install omapconf_install linux-dtbs_install ti-sgx-ddk-km_install cmem-mod_install pru-icss_install dual-camera-demo_install image-gallery_install uio-module-drv_install ti-gc320-driver_install ti-ipc_install barcode-roi_install linalg-examples_install 
# Kernel build targets
linux: linux-dtbs
	@echo =================================
	@echo     Building the Linux Kernel
	@echo =================================
	$(MAKE) -C $(LINUXKERNEL_INSTALL_DIR) ARCH=arm CROSS_COMPILE=$(CROSS_COMPILE) $(DEFCONFIG)
	$(MAKE) -j $(MAKE_JOBS) -C $(LINUXKERNEL_INSTALL_DIR) ARCH=arm CROSS_COMPILE=$(CROSS_COMPILE) zImage
	$(MAKE) -j $(MAKE_JOBS) -C $(LINUXKERNEL_INSTALL_DIR) ARCH=arm CROSS_COMPILE=$(CROSS_COMPILE) modules

linux_install: linux-dtbs_install
	@echo ===================================
	@echo     Installing the Linux Kernel
	@echo ===================================
	@if [ ! -d $(DESTDIR) ] ; then \
		echo "The extracted target filesystem directory doesn't exist."; \
		echo "Please run setup.sh in the SDK's root directory and then try again."; \
		exit 1; \
	fi
	install -d $(DESTDIR)/boot
	install $(LINUXKERNEL_INSTALL_DIR)/arch/arm/boot/zImage $(DESTDIR)/boot
	install $(LINUXKERNEL_INSTALL_DIR)/vmlinux $(DESTDIR)/boot
	install $(LINUXKERNEL_INSTALL_DIR)/System.map $(DESTDIR)/boot
	$(MAKE) -C $(LINUXKERNEL_INSTALL_DIR) ARCH=arm CROSS_COMPILE=$(CROSS_COMPILE) INSTALL_MOD_PATH=$(DESTDIR) INSTALL_MOD_STRIP=$(INSTALL_MOD_STRIP) modules_install

linux_clean:
	@echo =================================
	@echo     Cleaning the Linux Kernel
	@echo =================================
	$(MAKE) -C $(LINUXKERNEL_INSTALL_DIR) ARCH=arm CROSS_COMPILE=$(CROSS_COMPILE) mrproper
# Make Rules for matrix-gui project
matrix-gui:
	@echo =============================
	@echo      Building Matrix GUI
	@echo =============================
	@echo    NOTHING TO DO.  COMPILATION NOT REQUIRED

matrix-gui_clean:
	@echo =============================
	@echo      Cleaning Matrix GUI
	@echo =============================
	@echo    NOTHING TO DO.

matrix-gui_install:
	@echo =============================
	@echo     Installing Matrix GUI
	@echo =============================
	@cd example-applications; cd `find . -name "*matrix-gui-2.0*"`; make install
# arm-benchmarks build targets
arm-benchmarks:
	@echo =============================
	@echo    Building ARM Benchmarks
	@echo =============================
	@cd example-applications; cd `find . -name "*arm-benchmarks*"`; make

arm-benchmarks_clean:
	@echo =============================
	@echo    Cleaning ARM Benchmarks
	@echo =============================
	@cd example-applications; cd `find . -name "*arm-benchmarks*"`; make clean

arm-benchmarks_install:
	@echo ==============================================
	@echo   Installing ARM Benchmarks - Release version
	@echo ==============================================
	@cd example-applications; cd `find . -name "*arm-benchmarks*"`; make install

arm-benchmarks_install_debug:
	@echo ============================================
	@echo   Installing ARM Benchmarks - Debug Version
	@echo ============================================
	@cd example-applications; cd `find . -name "*arm-benchmarks*"`; make install_debug
# am-sysinfo build targets
am-sysinfo:
	@echo =============================
	@echo    Building AM Sysinfo
	@echo =============================
	@cd example-applications; cd `find . -name "*am-sysinfo*"`; make

am-sysinfo_clean:
	@echo =============================
	@echo    Cleaning AM Sysinfo
	@echo =============================
	@cd example-applications; cd `find . -name "*am-sysinfo*"`; make clean

am-sysinfo_install:
	@echo ===============================================
	@echo     Installing AM Sysinfo - Release version
	@echo ===============================================
	@cd example-applications; cd `find . -name "*am-sysinfo*"`; make install

am-sysinfo_install_debug:
	@echo =============================================
	@echo     Installing AM Sysinfo - Debug version
	@echo =============================================
	@cd example-applications; cd `find . -name "*am-sysinfo*"`; make install_debug
# oprofile-example build targets
oprofile-example:
	@echo =============================
	@echo    Building OProfile Example
	@echo =============================
	@cd example-applications; cd `find . -name "*oprofile-example*"`; make

oprofile-example_clean:
	@echo =============================
	@echo    Cleaning OProfile Example
	@echo =============================
	@cd example-applications; cd `find . -name "*oprofile-example*"`; make clean

oprofile-example_install:
	@echo =============================================
	@echo     Installing OProfile Example - Debug version
	@echo =============================================
	@cd example-applications; cd `find . -name "*oprofile-example*"`; make install
# matrix-gui-browser build targets
matrix-gui-browser:
	@echo =================================
	@echo    Building Matrix GUI Browser
	@echo =================================
	@cd example-applications; cd `find . -name "*matrix-gui-browser*"`; make -f Makefile.build release

matrix-gui-browser_clean:
	@echo =================================
	@echo    Cleaning Matrix GUI Browser
	@echo =================================
	@cd example-applications; cd `find . -name "*matrix-gui-browser*"`; make -f Makefile.build clean

matrix-gui-browser_install:
	@echo ===================================================
	@echo   Installing Matrix GUI Browser - Release version
	@echo ===================================================
	@cd example-applications; cd `find . -name "*matrix-gui-browser*"`; make -f Makefile.build install

matrix-gui-browser_install_debug:
	@echo =================================================
	@echo   Installing Matrix GUI Browser - Debug Version
	@echo =================================================
	@cd example-applications; cd `find . -name "*matrix-gui-browser*"`; make -f Makefile.build install_debug
# refresh-screen build targets
refresh-screen:
	@echo =============================
	@echo    Building Refresh Screen
	@echo =============================
	@cd example-applications; cd `find . -name "*refresh-screen*"`; make -f Makefile.build release

refresh-screen_clean:
	@echo =============================
	@echo    Cleaning Refresh Screen
	@echo =============================
	@cd example-applications; cd `find . -name "*refresh-screen*"`; make -f Makefile.build clean

refresh-screen_install:
	@echo ================================================
	@echo   Installing Refresh Screen - Release version
	@echo ================================================
	@cd example-applications; cd `find . -name "*refresh-screen*"`; make -f Makefile.build install

refresh-screen_install_debug:
	@echo ==============================================
	@echo   Installing Refresh Screen - Debug Version
	@echo ==============================================
	@cd example-applications; cd `find . -name "*refresh-screen*"`; make -f Makefile.build install_debug
cryptodev: linux
	@echo ================================
	@echo      Building cryptodev-linux
	@echo ================================
	@cd board-support/extra-drivers; \
	cd `find . -maxdepth 1 -name "cryptodev*"`; \
	make ARCH=arm KERNEL_DIR=$(LINUXKERNEL_INSTALL_DIR)

cryptodev_clean:
	@echo ================================
	@echo      Cleaning cryptodev-linux
	@echo ================================
	@cd board-support/extra-drivers; \
	cd `find . -maxdepth 1 -name "cryptodev*"`; \
	make ARCH=arm KERNEL_DIR=$(LINUXKERNEL_INSTALL_DIR) clean

cryptodev_install:
	@echo ================================
	@echo      Installing cryptodev-linux
	@echo ================================
	@if [ ! -d $(DESTDIR) ] ; then \
		echo "The extracted target filesystem directory doesn't exist."; \
		echo "Please run setup.sh in the SDK's root directory and then try again."; \
		exit 1; \
	fi
	@cd board-support/extra-drivers; \
	cd `find . -maxdepth 1 -name "cryptodev*"`; \
	make ARCH=arm  KERNEL_DIR=$(LINUXKERNEL_INSTALL_DIR)  INSTALL_MOD_PATH=$(DESTDIR) PREFIX=$(SDK_PATH_TARGET) INSTALL_MOD_STRIP=$(INSTALL_MOD_STRIP) install
debugss-module-drv: linux
	@echo =======================================
	@echo      Building debugss-module-drv
	@echo =======================================
	@cd board-support/extra-drivers; \
	cd `find . -maxdepth 1 -name "debugss*"`; \
	make ARCH=arm KERNEL_SRC=$(LINUXKERNEL_INSTALL_DIR) PLATFORM=DRA7xx_PLATFORM

debugss-module-drv_clean:
	@echo =======================================
	@echo      Cleaning debugss-module-drv
	@echo =======================================
	@cd board-support/extra-drivers; \
	cd `find . -maxdepth 1 -name "debugss*"`; \
	make ARCH=arm KERNEL_SRC=$(LINUXKERNEL_INSTALL_DIR) PLATFORM=DRA7xx_PLATFORM clean

debugss-module-drv_install:
	@echo =======================================
	@echo      Installing debugss-module-drv
	@echo =======================================
	@if [ ! -d $(DESTDIR) ] ; then \
		echo "The extracted target filesystem directory doesn't exist."; \
		echo "Please run setup.sh in the SDK's root directory and then try again."; \
		exit 1; \
	fi
	@cd board-support/extra-drivers; \
	cd `find . -maxdepth 1 -name "debugss*"`; \
	make ARCH=arm  KERNEL_SRC=$(LINUXKERNEL_INSTALL_DIR)  INSTALL_MOD_PATH=$(DESTDIR) PREFIX=$(SDK_PATH_TARGET) INSTALL_MOD_STRIP=$(INSTALL_MOD_STRIP) PLATFORM=DRA7xx_PLATFORM modules_install

gdbserverproxy-module-drv: linux
	@echo ==============================================
	@echo      Building gdbserverproxy-module-drv
	@echo ==============================================
	@cd board-support/extra-drivers; \
	cd `find . -maxdepth 1 -name "gdbserverproxy*"`; \
	make ARCH=arm KERNEL_SRC=$(LINUXKERNEL_INSTALL_DIR) PLATFORM=DRA7xx_PLATFORM

gdbserverproxy-module-drv_clean:
	@echo ==============================================
	@echo      Cleaning gdbserverproxy-module-drv
	@echo ==============================================
	@cd board-support/extra-drivers; \
	cd `find . -maxdepth 1 -name "gdbserverproxy*"`; \
	make ARCH=arm KERNEL_SRC=$(LINUXKERNEL_INSTALL_DIR) PLATFORM=DRA7xx_PLATFORM clean

gdbserverproxy-module-drv_install:
	@echo ==============================================
	@echo      Installing gdbserverproxy-module-drv
	@echo ==============================================
	@if [ ! -d $(DESTDIR) ] ; then \
		echo "The extracted target filesystem directory doesn't exist."; \
		echo "Please run setup.sh in the SDK's root directory and then try again."; \
		exit 1; \
	fi
	@cd board-support/extra-drivers; \
	cd `find . -maxdepth 1 -name "gdbserverproxy*"`; \
	make ARCH=arm  KERNEL_SRC=$(LINUXKERNEL_INSTALL_DIR)  INSTALL_MOD_PATH=$(DESTDIR) PREFIX=$(SDK_PATH_TARGET) INSTALL_MOD_STRIP=$(INSTALL_MOD_STRIP) PLATFORM=DRA7xx_PLATFORM modules_install

# OpenCL Examples build targets
opencl-examples:
	@echo =====================================
	@echo     Building the OpenCL Examples
	@echo =====================================
	@cd example-applications; cd `find . -name "*opencl-examples*"`; . $(ENV_SETUP); make TARGET_ROOTDIR=$(SDK_PATH_TARGET) TI_OCL_CGT_INSTALL=$(SDK_PATH_TARGET)/usr/share/ti/cgt-c6x

opencl-examples_install:
	@echo =======================================
	@echo     Installing the OpenCL Examples
	@echo =======================================
	@if [ ! -d $(DESTDIR) ] ; then \
		echo "The extracted target filesystem directory doesn't exist."; \
		echo "Please run setup.sh in the SDK's root directory and then try again."; \
		exit 1; \
	fi
	@cd example-applications; \
	cd `find . -name "*opencl-examples*"`; \
	for example in `find . -executable -not \( -type d -o -iname 'make*' \)`; do \
		install -d $(DESTDIR)/usr/share/ti/examples/opencl/`dirname $${example}` ; \
		install -m 755 $${example} $(DESTDIR)/usr/share/ti/examples/opencl/`dirname $${example}` ; \
		if [ -f `dirname $${example}`/*.out ]; then \
			install -m 755 `dirname $${example}`/*.out $(DESTDIR)/usr/share/ti/examples/opencl/`dirname $${example}` ; \
		fi \
	done

opencl-examples_clean:
	@echo =======================================
	@echo     Cleaning the OpenCL Examples
	@echo =======================================
	@cd example-applications; cd `find . -name "*opencl-examples*"`; . $(ENV_SETUP); make TARGET_ROOTDIR=$(SDK_PATH_TARGET) clean

# OpenMP Accelerator Examples build targets
openmpacc-examples:
	@echo ================================================
	@echo     Building the OpenMP Accelerator Examples
	@echo ================================================
	@cd example-applications; cd `find . -name "*openmpacc-examples*"`; . $(ENV_SETUP); make TARGET_ROOTDIR=$(SDK_PATH_TARGET) TI_OCL_CGT_INSTALL=$(SDK_PATH_TARGET)/usr/share/ti/cgt-c6x

openmpacc-examples_install:
	@echo ==================================================
	@echo     Installing the OpenMP Accelerator Examples
	@echo ==================================================
	@if [ ! -d $(DESTDIR) ] ; then \
		echo "The extracted target filesystem directory doesn't exist."; \
		echo "Please run setup.sh in the SDK's root directory and then try again."; \
		exit 1; \
	fi
	@cd example-applications; \
	cd `find . -name "*openmpacc-examples*"`; \
	for example in `find . -executable -not \( -type d -o -iname 'make*' \)`; do \
		install -d $(DESTDIR)/usr/share/ti/examples/openmpacc/`dirname $${example}` ; \
		install -m 755 $${example} $(DESTDIR)/usr/share/ti/examples/openmpacc/`dirname $${example}` ; \
		if [ -f `dirname $${example}`/*.out ]; then \
			install -m 755 `dirname $${example}`/*.out $(DESTDIR)/usr/share/ti/examples/openmpacc/`dirname $${example}` ; \
		fi \
	done

openmpacc-examples_clean:
	@echo ==================================================
	@echo     Cleaning the OpenMP Accelerator Examples
	@echo ==================================================
	@cd example-applications; cd `find . -name "*openmpacc-examples*"`; . $(ENV_SETUP); make TARGET_ROOTDIR=$(SDK_PATH_TARGET) clean

# qt-opencv-opencl-opengl-multithreaded  build targets
qt-opencv-opencl-opengl-multithreaded:
	@echo =============================
	@echo    Building QT OpenCV Multithreaded
	@echo =============================
	@cd example-applications; cd `find . -name "*qt-opencv-opencl-opengl-multithreaded*"`; make -f Makefile.build release

qt-opencv-opencl-opengl-multithreaded_clean:
	@echo =============================
	@echo    Cleaning QT OpenCV Multithreaded
	@echo =============================
	@cd example-applications; cd `find . -name "*qt-opencv-opencl-opengl-multithreaded*"`; make -f Makefile.build clean

qt-opencv-opencl-opengl-multithreaded_install:
	@echo ================================================
	@echo   Installing QT OpenCV Multithreaded - Release version
	@echo ================================================
	@cd example-applications; cd `find . -name "*qt-opencv-opencl-opengl-multithreaded*"`; make -f Makefile.build install

qt-opencv-opencl-opengl-multithreaded_install_debug:
	@echo ==============================================
	@echo   Installing QT OpenCV Multithreaded - Debug Version
	@echo ==============================================
	@cd example-applications; cd `find . -name "*qt-opencv-opencl-opengl-multithreaded*"`; make -f Makefile.build install_debug
# u-boot build targets
u-boot-spl: u-boot
u-boot-spl_clean: u-boot_clean
u-boot-spl_install: u-boot_install

u-boot: linux-dtbs
	@echo ===================================
	@echo    Building U-boot
	@echo ===================================
	$(MAKE) -j $(MAKE_JOBS) -C $(TI_SDK_PATH)/board-support/u-boot-* CROSS_COMPILE=$(CROSS_COMPILE) $(UBOOT_MACHINE)
	$(MAKE) -j $(MAKE_JOBS) -C $(TI_SDK_PATH)/board-support/u-boot-* CROSS_COMPILE=$(CROSS_COMPILE) DTC=$(LINUXKERNEL_INSTALL_DIR)/scripts/dtc/dtc

u-boot_clean:
	@echo ===================================
	@echo    Cleaining U-boot
	@echo ===================================
	$(MAKE) -C $(TI_SDK_PATH)/board-support/u-boot-* CROSS_COMPILE=$(CROSS_COMPILE) distclean

u-boot_install:
	@echo ===================================
	@echo    Installing U-boot
	@echo ===================================
	@echo "Nothing to do"
# omapconf build targets
omapconf:
	@echo =======================
	@echo    Building OmapConf
	@echo =======================
	@cd example-applications; cd `find . -name "*omapconf*"`; make CROSS_COMPILE=${CROSS_COMPILE} omapconf

omapconf_clean:
	@echo =======================
	@echo    Cleaning OmapConf
	@echo =======================
	@cd example-applications; cd `find . -name "*omapconf*"`; make clean

omapconf_install:
	@echo ==========================================
	@echo   Installing OmapConf - Release version
	@echo ==========================================
	@cd example-applications; cd `find . -name "*omapconf*"`; make DESTDIR=${DESTDIR}/usr/bin install

# Kernel DTB build targets
linux-dtbs:
	@echo =====================================
	@echo     Building the Linux Kernel DTBs
	@echo =====================================
	$(MAKE) -C $(LINUXKERNEL_INSTALL_DIR) ARCH=arm CROSS_COMPILE=$(CROSS_COMPILE) $(DEFCONFIG)
	$(MAKE) -j $(MAKE_JOBS) -C $(LINUXKERNEL_INSTALL_DIR) ARCH=arm CROSS_COMPILE=$(CROSS_COMPILE) am57xx-beagle-x15.dtb am57xx-beagle-x15-revb1.dtb am57xx-evm.dtb am57xx-evm-reva3.dtb am571x-idk.dtb am572x-idk.dtb am571x-idk-lcd-osd.dtb am572x-idk-lcd-osd.dtb

linux-dtbs_install:
	@echo =======================================
	@echo     Installing the Linux Kernel DTBs
	@echo =======================================
	@if [ ! -d $(DESTDIR) ] ; then \
		echo "The extracted target filesystem directory doesn't exist."; \
		echo "Please run setup.sh in the SDK's root directory and then try again."; \
		exit 1; \
	fi
	install -d $(DESTDIR)/boot
	@cp -f $(LINUXKERNEL_INSTALL_DIR)/arch/arm/boot/dts/*.dtb $(DESTDIR)/boot/

linux-dtbs_clean:
	@echo =======================================
	@echo     Cleaning the Linux Kernel DTBs
	@echo =======================================
	@echo "Nothing to do"

# ti-sgx-ddk-km module
ti-sgx-ddk-km: linux
	@echo ================================
	@echo      Building ti-sgx-ddk-km
	@echo ================================
	@cd board-support/extra-drivers; \
	cd `find . -maxdepth 1 -name "ti-sgx-ddk-km*" -type d`; \
	make -C ./eurasia_km/eurasiacon/build/linux2/omap_linux ARCH=arm TARGET_PRODUCT=jacinto6evm KERNELDIR=$(LINUXKERNEL_INSTALL_DIR)

ti-sgx-ddk-km_clean:
	@echo ================================
	@echo      Cleaning ti-sgx-ddk-km
	@echo ================================
	@cd board-support/extra-drivers; \
	cd `find . -maxdepth 1 -name "ti-sgx-ddk-km*" -type d`; \
	make -C ./eurasia_km/eurasiacon/build/linux2/omap_linux ARCH=arm KERNELDIR=$(LINUXKERNEL_INSTALL_DIR) clean

ti-sgx-ddk-km_install:
	@echo ================================
	@echo      Installing ti-sgx-ddk-km
	@echo ================================
	@cd board-support/extra-drivers; \
	cd `find . -maxdepth 1 -name "ti-sgx-ddk-km*" -type d`; \
	cd ./eurasia_km/eurasiacon/binary2_omap_linux_release/target/kbuild; \
	make -C $(LINUXKERNEL_INSTALL_DIR) SUBDIRS=`pwd` INSTALL_MOD_PATH=$(DESTDIR) PREFIX=$(SDK_PATH_TARGET) INSTALL_MOD_STRIP=$(INSTALL_MOD_STRIP) modules_install
# CMEM module
cmem-mod: linux
	@echo ================================
	@echo     Building CMEM Module
	@echo ================================
	@cd board-support/extra-drivers; \
	cd `find . -maxdepth 1 -name "cmem-mod*"`; \
	. $(ENV_SETUP); \
	make -f lu.mak ARCH=arm KERNEL_INSTALL_DIR=$(LINUXKERNEL_INSTALL_DIR) module

cmem-mod_clean:
	@echo ================================
	@echo     Cleaning CMEM Module
	@echo ================================
	@cd board-support/extra-drivers; \
	cd `find . -maxdepth 1 -name "cmem-mod*"`; \
	. $(ENV_SETUP); \
	make -f lu.mak ARCH=arm KERNEL_INSTALL_DIR=$(LINUXKERNEL_INSTALL_DIR) modules_clean

cmem-mod_install:
	@echo ================================
	@echo     Installing CMEM Module
	@echo ================================
	@if [ ! -d $(DESTDIR) ] ; then \
	echo "The extracted target filesystem directory doesn't exist."; \
		echo "Please run setup.sh in the SDK's root directory and then try again."; \
		exit 1; \
	fi
	@cd board-support/extra-drivers; \
	cd `find . -maxdepth 1 -name "cmem-mod*"`; \
	. $(ENV_SETUP); \
	make -C $(LINUXKERNEL_INSTALL_DIR) SUBDIRS=`pwd`/src/cmem/module INSTALL_MOD_PATH=$(DESTDIR) PREFIX=$(SDK_PATH_TARGET) INSTALL_MOD_STRIP=$(INSTALL_MOD_STRIP) modules_install


# PRU-ICSS build targets
pru-icss:
	@echo =================================
	@echo    Building PRU-ICSS
	@echo =================================
	@cd example-applications; cd `find . -name "*pru-icss*"`; \
	for dir in examples pru_cape/pru_fw lib/src labs; \
	do \
		make -C $$dir PRU_CGT=$(LINUX_DEVKIT_PATH)/sysroots/x86_64-arago-linux/usr/share/ti/cgt-pru; \
	done

pru-icss_clean:
	@echo =================================
	@echo    Cleaning PRU-ICSS
	@echo =================================
	@cd example-applications; cd `find . -name "*pru-icss*"`; \
	for dir in examples pru_cape/pru_fw lib/src labs; \
	do \
		make -C $$dir clean PRU_CGT=$(LINUX_DEVKIT_PATH)/sysroots/x86_64-arago-linux/usr/share/ti/cgt-pru; \
	done

pru-icss_install: pru-icss_install_am572x

pru-icss_install_common:
	@echo ===================================================
	@echo   Installing PRU-ICSS
	@echo ===================================================
	@if [ ! -d $(DESTDIR) ] ; then \
		echo "The extracted target filesystem directory doesn't exist."; \
		echo "Please run setup.sh in the SDK's root directory and then try again."; \
		exit 1; \
	fi
	@install -d $(DESTDIR)/lib/firmware/pru

pru-icss_install_none:
	@echo ===================================================
	@echo   Nothing to install
	@echo ===================================================

pru-icss_install_am335x: pru-icss_install_common
	@cd example-applications; cd `find . -name "*pru-icss*"`; \
	install -m 0644 ./examples/am335x/PRU_Halt/gen/PRU_Halt.out \
		$(DESTDIR)/lib/firmware/pru; \
	for i in 0 1; \
	do \
		install -m 0644 ./examples/am335x/PRU_RPMsg_Echo_Interrupt$${i}/gen/PRU_RPMsg_Echo_Interrupt$${i}.out \
			$(DESTDIR)/lib/firmware/pru; \
	done

pru-icss_install_am437x: pru-icss_install_common
	@cd example-applications; cd `find . -name "*pru-icss*"`; \
	install -m 0644 ./examples/am437x/PRU_Halt/gen/PRU_Halt.out \
		$(DESTDIR)/lib/firmware/pru; \
	for i in 0 1; \
	do \
		install -m 0644 ./examples/am437x/PRU_RPMsg_Echo_Interrupt$${i}/gen/PRU_RPMsg_Echo_Interrupt$${i}.out \
			$(DESTDIR)/lib/firmware/pru; \
	done

pru-icss_install_am572x: pru-icss_install_common
	@cd example-applications; cd `find . -name "*pru-icss*"`; \
	install -m 0644 ./examples/am572x/PRU_Halt/gen/PRU_Halt.out \
		$(DESTDIR)/lib/firmware/pru; \
	for i in 1 2; \
	do \
		for j in 0 1; \
		do \
			install -m 0644 ./examples/am572x/PRU_RPMsg_Echo_Interrupt$${i}_$${j}/gen/PRU_RPMsg_Echo_Interrupt$${i}_$${j}.out \
				$(DESTDIR)/lib/firmware/pru; \
		done; \
	done

# QT Dual Camera build targets
dual-camera-demo:
	@echo ================================
	@echo    Building Dual Camera Demo
	@echo ================================
	@cd example-applications; cd `find . -name "*dual-camera-demo*"`; make -f Makefile.build

dual-camera-demo_clean:
	@echo ================================
	@echo    Building Dual Camera Demo
	@echo ================================
	@cd example-applications; cd `find . -name "*dual-camera-demo*"`; make -f Makefile.build clean

dual-camera-demo_install:
	@echo ===================================================
	@echo   Installing Dual Camera Demo - Release version
	@echo ===================================================
	@cd example-applications; cd `find . -name "*dual-camera-demo*"`; make -f Makefile.build install

dual-camera-demo_install_debug:
	@echo =================================================
	@echo   Installing Dual Camera Demo - Debug version
	@echo =================================================
	@cd example-applications; cd `find . -name "*dual-camera-demo*"`; make -f Makefile.build install_debug
# QT Image Gallery build targets
image-gallery:
	@echo ================================
	@echo    Building Image Gallery
	@echo ================================
	@cd example-applications; cd `find . -name "*image-gallery*"`; make -f Makefile.build

image-gallery_clean:
	@echo ================================
	@echo    Building Image Gallery
	@echo ================================
	@cd example-applications; cd `find . -name "*image-gallery*"`; make -f Makefile.build clean

image-gallery_install:
	@echo ===================================================
	@echo   Installing Image Gallery - Release version
	@echo ===================================================
	@cd example-applications; cd `find . -name "*image-gallery*"`; make -f Makefile.build install

image-gallery_install_debug:
	@echo =================================================
	@echo   Installing Dual Camera Demo - Debug version
	@echo =================================================
	@cd example-applications; cd `find . -name "*image-galleryo*"`; make -f Makefile.build install_debug
uio-module-drv: linux
	@echo ================================
	@echo      Building uio-module-drv
	@echo ================================
	@cd board-support/extra-drivers; \
	cd `find . -maxdepth 1 -name "uio-module-drv*"`; \
	make ARCH=arm KERNEL_SRC=$(LINUXKERNEL_INSTALL_DIR)

uio-module-drv_clean:
	@echo ================================
	@echo      Cleaning uio-module-drv
	@echo ================================
	@cd board-support/extra-drivers; \
	cd `find . -maxdepth 1 -name "uio-module-drv*"`; \
	make ARCH=arm KERNEL_SRC=$(LINUXKERNEL_INSTALL_DIR) clean

uio-module-drv_install:
	@echo ================================
	@echo      Installing uio-module-drv
	@echo ================================
	@if [ ! -d $(DESTDIR) ] ; then \
		echo "The extracted target filesystem directory doesn't exist."; \
		echo "Please run setup.sh in the SDK's root directory and then try again."; \
		exit 1; \
	fi
	@cd board-support/extra-drivers; \
	cd `find . -maxdepth 1 -name "uio-module-drv*"`; \
	make ARCH=arm  KERNEL_SRC=$(LINUXKERNEL_INSTALL_DIR)  INSTALL_MOD_PATH=$(DESTDIR) PREFIX=$(SDK_PATH_TARGET) INSTALL_MOD_STRIP=$(INSTALL_MOD_STRIP) modules_install
# ti-gc320-driver module
ti-gc320-driver: linux
	@echo ================================
	@echo      Building ti-gc320-driver
	@echo ================================
	@cd board-support/extra-drivers; \
	cd `find . -maxdepth 1 -name "ti-gc320-driver*" -type d`; \
	make -f Kbuild AQROOT=`pwd` KERNEL_DIR=${LINUXKERNEL_INSTALL_DIR} ARCH_TYPE=arm

ti-gc320-driver_clean:
	@echo ================================
	@echo      Cleaning ti-gc320-driver
	@echo ================================
	@cd board-support/extra-drivers; \
	cd `find . -maxdepth 1 -name "ti-gc320-driver*" -type d`; \
	make -f Kbuild AQROOT=`pwd` KERNEL_DIR=${LINUXKERNEL_INSTALL_DIR} ARCH_TYPE=arm clean

ti-gc320-driver_install:
	@echo ================================
	@echo      Installing ti-gc320-driver
	@echo ================================
	@cd board-support/extra-drivers; \
	cd `find . -maxdepth 1 -name "ti-gc320-driver*" -type d`; \
	make -C $(LINUXKERNEL_INSTALL_DIR) SUBDIRS=`pwd` INSTALL_MOD_PATH=$(DESTDIR) PREFIX=$(SDK_PATH_TARGET) INSTALL_MOD_STRIP=$(INSTALL_MOD_STRIP) modules_install

TISDK_VERSION=03.02.00.05

PRSDK_VERSION=$(shell echo $(TISDK_VERSION) | sed -e 's|^0||' -e 's|\.|_|g')
PRSDK_PLATFORM=$(shell echo $(PLATFORM) | sed -e 's|-evm$$||')

ifneq ($(TI_RTOS_PATH),)
  TI_IPC_TARGETS = ti-ipc-linux

  TI_RTOS_PATH_ABS := $(realpath $(TI_RTOS_PATH))

  ifneq ($(TI_CCS_PATH),)
    TI_IPC_TARGETS += ti-ipc-linux-examples

    TI_CCS_PATH_ABS := $(realpath $(TI_CCS_PATH))
  else
    ifneq ($(wildcard $(TI_RTOS_PATH_ABS)/ccsv6),)
      TI_IPC_TARGETS += ti-ipc-linux-examples

      TI_CCS_PATH_ABS = $(TI_RTOS_PATH_ABS)/ccsv6
    else
      TI_IPC_TARGETS += ti-ipc-ccs-missing
    endif
  endif
else
  TI_IPC_TARGETS = ti-ipc-rtos-missing
endif

TI_IPC_CLEAN = $(addsuffix _clean, $(TI_IPC_TARGETS))
TI_IPC_INSTALL = $(addsuffix _install, $(TI_IPC_TARGETS))

export TI_PRSDK_PATH=$(TI_RTOS_PATH_ABS)/processor_sdk_rtos_$(PRSDK_PLATFORM)_$(PRSDK_VERSION)

export IPC_TOOLS_PATHS=gnu.targets.arm.A15=$${TOOLCHAIN_PATH_A15} gnu.targets.arm.A15F=$${TOOLCHAIN_PATH_A15} ti.targets.elf.C66=$${C6X_GEN_INSTALL_PATH} ti.targets.arm.elf.M4=$${TOOLCHAIN_PATH_M4} ti.targets.arm.elf.M4F=$${TOOLCHAIN_PATH_M4}

ti-ipc-rtos-missing ti-ipc-rtos-missing_clean ti-ipc-rtos-missing_install:
	@echo
	@echo ===========================================================
	@echo     If you wish to build IPC, please install
	@echo     Processor SDK RTOS $(TISDK_VERSION) for $(PLATFORM)
	@echo     and set TI_RTOS_PATH in Rules.make
	@echo ===========================================================

ti-ipc-ccs-missing ti-ipc-ccs-missing_clean ti-ipc-ccs-missing_install:
	@echo
	@echo ===========================================================
	@echo     If you wish to build IPC Examples, please install CCSv6
	@echo     and set TI_CCS_PATH in Rules.make
	@echo ===========================================================

ti-ipc-rtos-path-check:
	@if [ ! -d "$(TI_PRSDK_PATH)" ]; \
	then \
		echo; \
		echo "Error: TI_RTOS_PATH ($(TI_RTOS_PATH_ABS)) does not contain"; \
		echo "       the corresponding Processor SDK RTOS release!"; \
		echo; \
		echo "Please install Processor SDK RTOS $(TISDK_VERSION) for $(PLATFORM)."; \
		exit 1; \
	fi

ti-ipc: $(TI_IPC_TARGETS)

ti-ipc_clean: $(TI_IPC_CLEAN)

ti-ipc_install: $(TI_IPC_INSTALL)

ti-ipc-linux-config: ti-ipc-rtos-path-check
	@echo =================================
	@echo     Configuring IPC
	@echo =================================
	SDK_INSTALL_PATH=$(TI_RTOS_PATH_ABS) TOOLS_INSTALL_PATH=$(TI_CCS_PATH_ABS)/.. source $(TI_PRSDK_PATH)/setupenv.sh > /dev/null; \
	. $(ENV_SETUP); \
	cd $${IPC_INSTALL_PATH}; \
        ./configure $${CONFIGURE_FLAGS} \
		CC=$${CC} \
		--prefix=/usr \
		PLATFORM=$${IPC_PLATFORM} \
		KERNEL_INSTALL_DIR=$(LINUXKERNEL_INSTALL_DIR)


ti-ipc-linux: ti-ipc-rtos-path-check linux ti-ipc-linux-config
	@echo =================================
	@echo     Building IPC
	@echo =================================
	SDK_INSTALL_PATH=$(TI_RTOS_PATH_ABS) TOOLS_INSTALL_PATH=$(TI_CCS_PATH_ABS)/.. source $(TI_PRSDK_PATH)/setupenv.sh > /dev/null; \
	source $(ENV_SETUP); \
	$(MAKE) -j $(MAKE_JOBS) -C $${IPC_INSTALL_PATH}

ti-ipc-linux_clean: ti-ipc-rtos-path-check ti-ipc-linux-config
	@echo =================================
	@echo     Cleaning IPC
	@echo =================================
	SDK_INSTALL_PATH=$(TI_RTOS_PATH_ABS) TOOLS_INSTALL_PATH=$(TI_CCS_PATH_ABS)/.. source $(TI_PRSDK_PATH)/setupenv.sh > /dev/null; \
	source $(ENV_SETUP); \
	$(MAKE) -j $(MAKE_JOBS) -C $${IPC_INSTALL_PATH} clean

ti-ipc-linux_install: ti-ipc-rtos-path-check ti-ipc-linux
	@echo =================================
	@echo     Installing IPC
	@echo =================================
	@if [ ! -d $(DESTDIR) ] ; then \
		echo "The extracted target filesystem directory doesn't exist."; \
		echo "Please run setup.sh in the SDK's root directory and then try again."; \
		exit 1; \
	fi
	SDK_INSTALL_PATH=$(TI_RTOS_PATH_ABS) TOOLS_INSTALL_PATH=$(TI_CCS_PATH_ABS)/.. source $(TI_PRSDK_PATH)/setupenv.sh > /dev/null; \
	source $(ENV_SETUP); \
	$(MAKE) -j $(MAKE_JOBS) -C $${IPC_INSTALL_PATH} install DESTDIR=$(DESTDIR)

ti-ipc-linux-examples: ti-ipc-rtos-path-check
	@echo =================================
	@echo     Building the IPC Examples
	@echo =================================
	SDK_INSTALL_PATH=$(TI_RTOS_PATH_ABS) TOOLS_INSTALL_PATH=$(TI_CCS_PATH_ABS)/.. source $(TI_PRSDK_PATH)/setupenv.sh > /dev/null; \
	source $(ENV_SETUP); \
	$(MAKE) -j $(MAKE_JOBS) -C $${IPC_INSTALL_PATH}/examples \
		HOSTOS="linux" \
		PLATFORM="$${IPC_PLATFORM}" \
		KERNEL_INSTALL_DIR="$(LINUXKERNEL_INSTALL_DIR)" \
		XDC_INSTALL_DIR="$${XDC_INSTALL_PATH}" \
		BIOS_INSTALL_DIR="$${BIOS_INSTALL_PATH}" \
		IPC_INSTALL_DIR="$${IPC_INSTALL_PATH}" \
		TOOLCHAIN_LONGNAME=$${TOOLCHAIN_SYS} \
		TOOLCHAIN_INSTALL_DIR=$${SDK_PATH_NATIVE}/usr \
		TOOLCHAIN_PREFIX=$(CROSS_COMPILE) \
		$(IPC_TOOLS_PATHS)

ti-ipc-linux-examples_install: ti-ipc-rtos-path-check ti-ipc-linux-examples
	@echo =================================
	@echo     Installing the IPC Examples
	@echo =================================
	@if [ ! -d $(DESTDIR) ] ; then \
		echo "The extracted target filesystem directory doesn't exist."; \
		echo "Please run setup.sh in the SDK's root directory and then try again."; \
		exit 1; \
	fi
	SDK_INSTALL_PATH=$(TI_RTOS_PATH_ABS) TOOLS_INSTALL_PATH=$(TI_CCS_PATH_ABS)/.. source $(TI_PRSDK_PATH)/setupenv.sh > /dev/null; \
	source $(ENV_SETUP); \
	$(MAKE) -j $(MAKE_JOBS) -C $${IPC_INSTALL_PATH}/examples install \
		HOSTOS="linux" \
		PLATFORM="$${IPC_PLATFORM}" \
		EXEC_DIR="$(EXEC_DIR)"

ti-ipc-linux-examples_clean: ti-ipc-rtos-path-check
	@echo =================================
	@echo     Cleaning the IPC Examples
	@echo =================================
	SDK_INSTALL_PATH=$(TI_RTOS_PATH_ABS) TOOLS_INSTALL_PATH=$(TI_CCS_PATH_ABS)/.. source $(TI_PRSDK_PATH)/setupenv.sh > /dev/null; \
	source $(ENV_SETUP); \
	$(MAKE) -j $(MAKE_JOBS) -C $${IPC_INSTALL_PATH}/examples clean \
		HOSTOS="linux" \
		PLATFORM="$${IPC_PLATFORM}" \
		KERNEL_INSTALL_DIR="$(LINUXKERNEL_INSTALL_DIR)" \
		XDC_INSTALL_DIR="$${XDC_INSTALL_PATH}" \
		BIOS_INSTALL_DIR="$${BIOS_INSTALL_PATH}" \
		IPC_INSTALL_DIR="$${IPC_INSTALL_PATH}" \
		TOOLCHAIN_LONGNAME=$${TOOLCHAIN_SYS} \
		TOOLCHAIN_INSTALL_DIR=$${SDK_PATH_NATIVE}/usr \
		TOOLCHAIN_PREFIX=$(CROSS_COMPILE) \
		$(IPC_TOOLS_PATHS)

# barcode-roi build targets

barcode-roi:
	@echo =============================
	@echo    Building Barcode ROI
	@echo =============================
	@cd example-applications; cd `find . -name "*barcode-roi*"`; make -f Makefile.build release

barcode-roi_clean:
	@echo =============================
	@echo    Cleaning Barcode ROI
	@echo =============================
	@cd example-applications; cd `find . -name "*barcode-roi*"`; make -f Makefile.build clean

barcode-roi_install:
	@echo ================================================
	@echo   Installing Barcode ROI - Release version
	@echo ================================================
	@cd example-applications; cd `find . -name "*barcode-roi*"`; make -f Makefile.build install
linalg-examples:
	@echo =====================================
	@echo     Building the Linalg Examples
	@echo =====================================
	@cd example-applications; \
	cd `find . -name "*linalg-examples*"`; \
	. $(ENV_SETUP); \
	make TARGET_ROOTDIR=$(SDK_PATH_TARGET) \
		TI_OCL_CGT_INSTALL=$(SDK_PATH_TARGET)/usr/share/ti/cgt-c6x \
		LINALG_DIR=$(SDK_PATH_TARGET)/usr/share/ti/ti-linalg-tree

linalg-examples_install:
	@echo =======================================
	@echo     Installing the Linalg Examples
	@echo =======================================
	@if [ ! -d $(DESTDIR) ] ; then \
		echo "The extracted target filesystem directory doesn't exist."; \
		echo "Please run setup.sh in the SDK's root directory and then try again."; \
		exit 1; \
	fi
	@cd example-applications; \
	cd `find . -name "*linalg-examples*"`; \
	for example in `find . -executable -not \( -type d -o -iname 'make*' \)`; do \
		install -d $(DESTDIR)/usr/share/ti/examples/linalg/`dirname $${example}` ; \
		install -m 755 $${example} $(DESTDIR)/usr/share/ti/examples/linalg/`dirname $${example}` ; \
		if [ -f `dirname $${example}`/*.out ]; then \
			install -m 755 `dirname $${example}`/*.out $(DESTDIR)/usr/share/ti/examples/linalg/`dirname $${example}` ; \
		fi \
	done

linalg-examples_clean:
	@echo =======================================
	@echo     Cleaning the Linalg Examples
	@echo =======================================
	@cd example-applications; \
	cd `find . -name "*linalg-examples*"`; \
	. $(ENV_SETUP); \
	make TARGET_ROOTDIR=$(SDK_PATH_TARGET) clean

