#!/bin/bash
#Copyright (c) 2016 Texas Instruments
# Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
#The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
#THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

# script to build a basic root file system for wifi and bluetooth using 
# Processor SDK3.0.


# if writing direct to pre-existing SD card (1) or creating a full tar ball (0)
ROOTFS_ON_SD=0

# If ROOTFS_ON_SD is 1 this is mount point for the SD card on your Linux machine
MY_SD_MOUNT="/media/iain"


# Are we building mainline wilink8 drivers (1) or TI latest and full featured R8.7 out of tree (0) 
WILINK_MAINLINE=1

# set main directories in use
SDK_DIR=`pwd`


export NATIVE_PATH=x86_64-arago-linux
export TARGET_PATH=cortexa8hf-neon-linux-gnueabi
export CROSS_COMPILE="${SDK_DIR}/linux-devkit/sysroots/${NATIVE_PATH}/usr/bin/arm-linux-gnueabihf-"
export PATH="$PATH:${SDK_DIR}/linux-devkit/sysroots/${NATIVE_PATH}/usr/bin"
export USR_INCLUDE_PATH="${SDK_DIR}/linux-devkit/sysroots/${TARGET_PATH}/usr"


# Pick up the correct kernel version used in this particular SDK
KERNEL_VER_BASE=linux-4.4.12+gitAUTOINC+3639bea54a-g3639bea54a
UBOOT=u-boot-2016.05+gitAUTOINC+b4e185a8c3-gb4e185a8c3


# create a new kernel name for different boards.
# make handling multiple boards easier
export KERNEL_VER=${KERNEL_VER_BASE}-am57xx-evm



# now patch Rules.mak to handle the new directory name
if [ ! -f Rules.make.orig ]
then
   cp Rules.make Rules.make.orig
fi

sed "s/${KERNEL_VER_BASE}/${KERNEL_VER}/g" Rules.make.orig > Rules.make


# now create board specific file system
FS_NAME="sdk3-wilink-filesystem"
if [ ${ROOTFS_ON_SD} -eq 1 ] 
then
   FS="${MY_SD_MOUNT}/rootfs"
   export INSTALL_PATH="${FS}"
else

   FS="${FS_NAME}-am57xx-evm-r87"
   TAR_FS="tar-${FS_NAME}-am57xx-evm"

  export INSTALL_PATH="${SDK_DIR}/${FS}"

fi 


if [ ${ROOTFS_ON_SD} -eq 1 ] 
then
  # check that SD card is mounted before starting 
   if [ ! -e ${FS} ] 
   then
      echo "SD card not mounted"
      exit
   fi
fi

if [ ${ROOTFS_ON_SD} -eq 0 ] 
then
   # only clean FS when creating a new partition

   if [ ! -e ${FS} ]
   then
     mkdir ${FS}
   else
     cd ${FS}
     sudo rm -r *  
     cd ..
   fi
   if [ ! -e ${TAR_FS} ]
   then
     mkdir ${TAR_FS}
   fi


   # extract full image to get all wi-fi user space tools
   cd ${FS}
   sudo tar -xf ../filesystem/tisdk-rootfs-image-am57xx-evm.tar.gz .

   cd ..
fi

##### Patch the kernel as required
if [ ! -f kernel.patched ]
then

cd board-support/
#if a clean tar file does not exist then create one
if [ ! -e ${KERNEL_VER_BASE}.tar.gz ]
then
   tar -czf ${KERNEL_VER_BASE}.tar.gz ${KERNEL_VER_BASE}
   # and move current directory to a backup
   cp -r ${KERNEL_VER_BASE} ${KERNEL_VER_BASE}-clean
   mv ${KERNEL_VER_BASE} ${KERNEL_VER}

else
   # as tar file exists then delete working build and repopulate with clean tar 
   echo "extract tar kernel"
   sudo rm -r ${KERNEL_VER}
   tar -xf ${KERNEL_VER_BASE}.tar.gz
   mv ${KERNEL_VER_BASE} ${KERNEL_VER}
fi
cd ..


  # clean it just to be sure
  make linux_clean


  cd board-support/${KERNEL_VER} ||  exit

  git am ../../patches/kernel/0001-add-Wilink8-support-to-sdk3-defconfig.patch || exit
  git am ../../patches/kernel/0002-add-iptables-support.patch || exit
  git am ../../patches/kernel/0003-add-mmc3-for-wilink8-and-correct-mmc-2-addresses.patch || exit
 

  if [ ${WILINK_MAINLINE} -eq 0 ]
  then
     # add patch to disable built in wilink drivers in defconfig
     git am ../../patches/kernel/0004-disable-wifi-in-kernel-build.patch || exit
  fi

  cd ../..
  # create this file to say kernel has been patched. To recreate from scratch delete this file
  touch kernel.patched
fi



##### Make the kernel 
make linux MAKE_JOBS=8 || exit



#The main linux_install script uses Rules.mak to set filesystem. We are not using that directory here and so
# we will explicitly call the install routines so ${FS} can be passed
#sudo make linux_install
sudo install -d ${FS}/boot
sudo install board-support/${KERNEL_VER}/arch/arm/boot/zImage ${FS}/boot

# reduce filesystem size by not installing vmlinux (109Mb) and removing original zImage
  sudo rm ${FS}/boot/zImage-${KERNEL_VER}
  #remove this line for creating a smaller filesystem (vmlinux is 109Mb)
  #sudo install board-support/linux-3.14.26-g2489c02/vmlinux ${FS}/boot

sudo install board-support/${KERNEL_VER}/System.map ${FS}/boot
sudo cp -f board-support/${KERNEL_VER}/arch/arm/boot/dts/*.dtb ${FS}/boot/

if [ ${ROOTFS_ON_SD} -eq 1 ] 
then
   sudo make -C board-support/${KERNEL_VER} ARCH=arm CROSS_COMPILE=${CROSS_COMPILE} INSTALL_MOD_PATH=${INSTALL_PATH} modules_install  
else
   sudo make -C board-support/${KERNEL_VER} ARCH=arm CROSS_COMPILE=${CROSS_COMPILE} INSTALL_MOD_PATH=${INSTALL_PATH} modules_install  
fi

# If building latest drivers out of tree then run the build script
# http://processors.wiki.ti.com/index.php/WL18xx_System_Build_Scripts
if [ ${WILINK_MAINLINE} -eq 0 ]
then

   # build R8.7 out of tree and install into filesystem
   if [ ! -e wifi-downloaded ]
   then
      mkdir -p wifi-wl8
      cd wifi-wl8
      git clone git://git.ti.com/wilink8-wlan/build-utilites.git || exit
      cd build-utilites || exit
      git checkout master 
      cp ../../patches/wifi-build/setup-env-bbb setup-env || exit

      # Errors here referring to being unable to create directories are ok here. The filesystem is owned by root
      # and so user can't create some directories. This is not a problem as the actual build is run under sudo
      # We do not run the step as root as it would make root the owner of all the source files. 
      ./build_wl18xx.sh update R8.7 
   
      touch ../../wifi-downloaded
      cd ../..
   fi

   # now build and install wifi drivers
   cd wifi-wl8/build-utilites
   # do a clean and build
   . ./sudo_build_wl18xx.sh clean
   cd ../..
fi

cd ${INSTALL_PATH}



if [ ${WILINK_MAINLINE} -eq 1 ]
then
  #correct installation path of wlconf in SDK 3.0 if mainline used
  sudo mv usr/bin/wlconf usr/sbin

else
  # as TI script installs to correct location delete the second and wrong SDK wlconf
  sudo rm -r usr/bin/wlconf
fi

# use default wlconf configuration in driver with 2 2.4GHz and 1 5GHz antennae.
# is also is 40MHz mode so only SISO40 on 2.4GHz and no MIMO
# works on both wl1835 and wl1837 as mainline kernel does not support diversity
sudo rm lib/firmware/ti-connectivity/wl18xx-conf.bin

# correct file format problem with INI files
sudo dos2unix usr/sbin/wlconf/official_inis/*


if [ ${ROOTFS_ON_SD} -eq 0 ] 
then
   sudo tar -czf ../${TAR_FS}/rootfs_partition.tar.gz *


   # change to boot, and only remove files if that 'cd' command executed correctly 
   # remove existing mlo, u-boot from directory, we can't copy symbolic links to FAT  
   # ensure it is target filesystem boot and not host /boot !!!
   cd ../${FS}/boot && sudo rm -r *

   # as we are writing to a FAT ultimately symbolic links don't work, so remove them by writing to target names
   sudo cp ../../board-support/prebuilt-images/MLO-am57xx-evm MLO
   sudo cp ../../board-support/prebuilt-images/u-boot-am57xx-evm.img u-boot.img
   sudo cp ../../board-support/prebuilt-images/u-boot-spl.bin-am57xx-evm u-boot.bin
   sudo cp ../../board-support/prebuilt-images/uEnv.txt uEnv.txt


   sudo tar -czf ../../${TAR_FS}/boot_partition.tar.gz *
   cd ../..
else
   # just sync to SD card
   sync
   sync
   umount /media/iain/rootfs
   umount /media/iain/boot
fi 
