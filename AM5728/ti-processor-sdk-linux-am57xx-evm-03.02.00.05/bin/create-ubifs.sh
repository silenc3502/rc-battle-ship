#!/bin/bash

# This distribution contains contributions or derivatives under copyright
# as follows:
#
# Copyright (c) 2016, Texas Instruments Incorporated
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# - Redistributions of source code must retain the above copyright notice,
#   this list of conditions and the following disclaimer.
# - Redistributions in binary form must reproduce the above copyright
#   notice, this list of conditions and the following disclaimer in the
#   documentation and/or other materials provided with the distribution.
# - Neither the name of Texas Instruments nor the names of its
#   contributors may be used to endorse or promote products derived
#   from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
# TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
# CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
# PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
# LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
# OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
# ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

# This variable will be populated during installation
TI_SDK_PATH="/home/kwrg/gproj/RCBattleShip/ti-processor-sdk-linux-am57xx-evm-03.02.00.05"

# The following variables are populated during the SDK build
MKUBIFS_ARGS="-F -m 2048 -e 126976 -c 12288"
UBINIZE_ARGS="-m 2048 -p 128KiB -s 512 -O 2048"

machine=""
image_rootfs=""
image_name=""
vol_name=""


usage() {
    cat << __EOF__

Usage: sudo $0 <image_rootfs_dir> [-o <image_name_prefix>] [-v <vol_name>]

Example:
    sudo $0 ../targetNFS -o ../filesystem/custom-${machine}-rootfs

This command will create the UBI image at "../filesytem/custom-${machine}-rootfs.ubi"

__EOF__
}

default_args() {
    machine=`grep "^PLATFORM" "${TI_SDK_PATH}/Rules.make" | sed -e 's|^.*=[ \t]*||'`

    image_rootfs="${TI_SDK_PATH}/targetNFS"
    image_name="${TI_SDK_PATH}/filesystem/custom-${machine}-rootfs"
    vol_name="${machine}-rootfs"
}

parse_args() {
    while [ $# -gt 0 ]
    do
        opt="$1"
        shift
        case $opt in
            "-h"|"--help") usage; exit 0;;
            "-o") image_name="$1"; shift;;
            "-v") vol_name="$1"; shift;;
            -*) echo "Error: Invalid option: \"${opt}\""; usage; exit 1;;
            *)    image_rootfs="$opt";;
        esac
    done
}

check_config() {
    if [ ! -d "${TI_SDK_PATH}" ]
    then
        echo "Error: Cannot find original installation directory!"
        exit 1
    fi

    if [ ! -d "${image_rootfs}" ]
    then
        echo "Error: \"${image_rootfs}\" is not a valid directory for the image_rootfs!"
        usage
        exit 1
    fi

    AMIROOT=`whoami | awk {'print $1'}`
    if [ "$AMIROOT" != "root" ] ; then
        echo "Error: Must run script with sudo!"
        echo ""
        exit 1
    fi

    cat << __EOF__

Parameters:
    image_rootfs_dir ..... "${image_rootfs}"
    image_name_prefix .... "${image_name}"
    vol_name ............. "${vol_name}"

__EOF__
}

create_ubinize_cfg() {
    cat >> "${image_name}.ubinize.cfg" << __EOF__
[ubifs]
mode=ubi
image=${image_name}.ubifs
vol_id=0
vol_type=dynamic
vol_name=${vol_name}
vol_flags=autoresize
__EOF__
}

default_args
parse_args "$@"
check_config


source ${TI_SDK_PATH}/linux-devkit/environment-setup

PATH=${SDK_PATH_NATIVE}/usr/sbin:$PATH

mkfs.ubifs -v -r "${image_rootfs}" -o "${image_name}.ubifs" ${MKUBIFS_ARGS}
if [ $? -ne 0 ]
then
    echo ""
    echo "Error: mkfs.ubifs failed!"
    exit 1
fi

create_ubinize_cfg

ubinize -v -o "${image_name}.ubi" ${UBINIZE_ARGS} "${image_name}.ubinize.cfg"
if [ $? -ne 0 ]
then
    echo ""
    echo "Error: ubinize failed!"
    exit 1
fi

echo ""
echo "Success! The UBI image is available at \"${image_name}.ubi\""
echo ""

exit 0
