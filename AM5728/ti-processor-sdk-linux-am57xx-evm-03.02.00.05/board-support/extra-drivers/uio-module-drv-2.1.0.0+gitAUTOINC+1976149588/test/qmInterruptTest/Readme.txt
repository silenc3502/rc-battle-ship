 ******************************************************************************
 * FILE PURPOSE: Readme File for the QMSS Infrastructure Multicore Mode Example Project
 ******************************************************************************
 * FILE NAME: Readme.txt
 * Copyright (C) 2009, Texas Instruments, Inc.
 *****************************************************************************

The example demonstrates data transfer and synchronization between multiple cores. 
The example has to be loaded on all 4 cores. It uses accumulation to trigger interrupts to the host. 
The example is provided for both Big and Little Endian. 

Check the release notes for prerequisites, version information and steps on how to run examples

The PDSP firmware downloded is 48 channel little endian firmware. For big endian change to corresponding firmware 
e.g., change acc48_le to acc48_be


======================
Copy files under ti/drv/qmss/example
cd qmInterruptTest/k2h/armv7/linux/build
edit setenv.sh to point to required directories
source setenv.sh
make
======================
