#! /bin/sh

# just helping for cross compilation.

MACHINE=evbsh3
MACHINE_ARCH=sh3el
# just in case you forgot to specify this when you built gcc...
CFLAGS="-D__NetBSD__ -v"
export MACHINE MACHINE_ARCH CFLAGS
TARGET=/usr/pkg/cross/shel-netbsdcoff

make AR=$TARGET-ar AS=$TARGET-as CC=$TARGET-gcc LD=$TARGET-ld NM=$TARGET-nm \
	RANLIB=$TARGET-ranlib SIZE=$TARGET-size \
	STRIP=$TARGET-strip OBJCOPY=$TARGET-objcopy \
	CPP=$TARGET-cpp CXX=$TARGET-c++ $*
