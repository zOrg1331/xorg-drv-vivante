#!/bin/sh

#Build Configuration
#-------------------
export AQROOT=$PWD/../../..
export LDFLAGSVIV="${AQROOT}/build/sdk/drivers -lGAL -lm -ldl"
export CFLAGS='-I${AQROOT}/build/sdk/include -L${LDFLAGSVIV}'

./autogen.sh --prefix=/usr --libdir '/usr/lib'  --disable-static

echo "End of configuration"

