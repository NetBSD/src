#!/bin/sh

# This assumes the compiler comes from pkgsrc/cross/gcc-mips-current
# (as R5900 support is not available on other branches of gcc yet)
root=/usr/pkg
target=mipsel--netbsd

EXTERNAL_TOOLCHAIN=${root};		export EXTERNAL_TOOLCHAIN
LD=${root}/bin/${target}-ld;		export LD
CC=${root}/bin/${target}-gcc;		export CC
CXX=${root}/bin/${target}-g++;		export CXX
AS=${root}/bin/${target}-as;		export AS
CPP=${root}/bin/${target}-cpp;		export CPP
RANLIB=${root}/bin/${target}-ranlib;	export RANLIB
AR=${root}/bin/${target}-ar;		export AR
NM=${root}/bin/${target}-nm;		export NM
SIZE=${root}/bin/${target}-size;	export SIZE
STRIP=${root}/bin/${target}-strip;	export STRIP
OBJCOPY=${root}/bin/${target}-objcopy;	export OBJCOPY

MAKE="make";				 export MAKE

exec $MAKE "$@"
