#!/bin/sh

#Build Configuration
#-------------------
export CFLAGS='-I../../EXA/src/vivante_gal -g -Wa,-mimplicit-it=thumb -lm -ldl -ldrm -lX11 -I../../../../build/sdk/include'

./autogen.sh --prefix=/usr --libdir '/usr/lib'  --disable-static

echo "End of configuration"
