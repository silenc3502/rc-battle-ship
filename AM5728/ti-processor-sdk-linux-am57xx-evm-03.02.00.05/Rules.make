#platform
PLATFORM=am57xx-evm

#defconfig
DEFCONFIG=tisdk_am57xx-evm_defconfig

#Architecture
ARCH=armv7-a

#u-boot machine
UBOOT_MACHINE=am57xx_evm_config

#Points to the root of the TI SDK
export TI_SDK_PATH=/home/kwrg/gproj/RCBattleShip/ti-processor-sdk-linux-am57xx-evm-03.02.00.05

#root of the target file system for installing applications
DESTDIR=__DESTDIR__

#Points to the root of the Linux libraries and headers matching the
#demo file system.
export LINUX_DEVKIT_PATH=$(TI_SDK_PATH)/linux-devkit

#Cross compiler prefix
export CROSS_COMPILE=$(LINUX_DEVKIT_PATH)/sysroots/x86_64-arago-linux/usr/bin/arm-linux-gnueabihf-

#Default CC value to be used when cross compiling.  This is so that the
#GNU Make default of "cc" is not used to point to the host compiler
export CC=$(CROSS_COMPILE)gcc

#Location of environment-setup file
export ENV_SETUP=$(LINUX_DEVKIT_PATH)/environment-setup

#The directory that points to the SDK kernel source tree
LINUXKERNEL_INSTALL_DIR=$(TI_SDK_PATH)/board-support/linux-4.4.32+gitAUTOINC+adde2ca9f8-gadde2ca9f8

CFLAGS= -march=armv7-a -marm -mfpu=neon  -mfloat-abi=hard

#Strip modules when installing to conserve disk space
INSTALL_MOD_STRIP=1

SDK_PATH_TARGET=$(TI_SDK_PATH)/linux-devkit/sysroots/armv7ahf-neon-linux-gnueabi/

# Set EXEC_DIR to install example binaries
EXEC_DIR=__EXEC_DIR__

MAKE_JOBS=4
