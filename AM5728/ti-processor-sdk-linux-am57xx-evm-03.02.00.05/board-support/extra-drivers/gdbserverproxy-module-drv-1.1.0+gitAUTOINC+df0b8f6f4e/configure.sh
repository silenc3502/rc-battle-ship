#!/bin/sh

opt=$1
 
case "${opt}" in
    -add)			
    	# create dspdbg group if it isn't already there
    	if ! getent group dspdbg >/dev/null; then
	    echo "Creating dspdbg group."
	    addgroup --quiet --system dspdbg
   	fi
	;;
    *)
	echo ERROR: invalid configure.sh option: "${opt}"
	exit 1
	;;
esac			
exit 0
