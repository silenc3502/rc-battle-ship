#!/bin/bash
export CROSS_TOOL_INSTALL_PATH=~/gcc-linaro-arm-linux-gnueabihf-4.7-2013.02-01-20130221_linux/bin
export CROSS_TOOL_PRFX=arm-linux-gnueabihf-
export PDK_INSTALL_PATH=/home/a0862859/yocto_mcsdk304/oe-layersetup/build/arago-tmp-external-linaro-toolchain/sysroots/keystone-evm/usr/include
export DEVICE=k2h
export ARMV7OBJDIR=./obj/${DEVICE}
export ARMV7BINDIR=./bin/${DEVICE}
export QMSS_SRC_DIR=~/yocto_mcsdk304/oe-layersetup/build/arago-tmp-external-linaro-toolchain/work/cortexa15hf-vfp-neon-3.8-oe-linux-gnueabi/ti-qmss-bin/02.00.00.18-r0/git/ti/drv/qmss
export LDFLAGS=--sysroot=/home/a0862859/yocto_mcsdk304/oe-layersetup/build/arago-tmp-external-linaro-toolchain/sysroots/keystone-evm
