#!/bin/sh

#Copyright (C) 2011 Texas Instruments Incorporated - http://www.ti.com/ 
#
# 
# Redistribution and use in source and binary forms, with or without 
# modification, are permitted provided that the following conditions 
# are met:
#
# Redistributions of source code must retain the above copyright 
# notice, this list of conditions and the following disclaimer.
#
# Redistributions in binary form must reproduce the above copyright
# notice, this list of conditions and the following disclaimer in the 
# documentation and/or other materials provided with the   
# distribution.
#
# Neither the name of Texas Instruments Incorporated nor the names of
# its contributors may be used to endorse or promote products derived
# from this software without specific prior written permission.
# 
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

# Important System Environment Variables
SCRIPTS=/etc/profile.d/*.sh

for f in $SCRIPTS
do
    source $f
done

mkdir -p tmp
mkdir -p lock

i=0

for item in "$@"
do

  if [ $i -eq 0 ]
  then
      
	filename=$item   
  fi
 
  if [ $i -eq 1 ] 
  then
        
	outputfilename=$item 
  fi

  if [ $i -eq 2 ]
  then
    program_type=$item
  fi

  if [ $i -eq 3 ]
  then
    category=$item
  fi

  if [ $i -gt 3 ]
  then
    touch "lock/"$item

   fi
 i=`expr $i + 1`

done

if [ $program_type == gui ]
then
  /etc/init.d/matrix-gui-2.0 stop

  if [ $category == 3d -o $category == multimedia -o $category == touch ]
  then
    /etc/init.d/weston stop
    sleep 1
  fi
fi

echo "Filename:"$filename
echo "Output:"$outputfilename
eval $filename > "tmp/"$outputfilename 2>&1

if [ $program_type == gui ]
then
  if [ $category == 3d -o $category == multimedia -o $category == touch ]
  then
    /etc/init.d/weston start
    sleep 1
  fi

  /etc/init.d/matrix-gui-2.0 start
fi

#Using a more unique string to detect if the script is completed
echo "_?!!MATRIX_SCRIPT_COMPLETED!!?_" >> "tmp/"$outputfilename

i=0

for item in "$@"
do

  if [ $i -gt 1 ]
  then
    rm -f "lock/"$item

   fi
 i=`expr $i + 1`

done

sleep 20
rm "tmp/"$outputfilename

