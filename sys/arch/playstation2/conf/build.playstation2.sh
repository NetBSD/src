#!/bin/sh
DESTDIR=/work/playstation2/root;	export DESTDIR
RELEASEDIR=/work/playstation2/release;	export RELEASEDIR
root=/usr/pkg/cross-ps2
target=mipsEEel-netbsd

PATH=\
$PATH:\
${root}/${target}/bin:\
${root}/bin
export PATH

CC=${root}/bin/${target}-gcc;		export CC
LD=${root}/bin/${target}-ld;		export LD
CXX=${root}/bin/${target}-g++;		export CXX
AS=${root}/bin/${target}-as;		export AS
CPP=${root}/bin/${target}-cpp;		export CPP
RANLIB=${root}/bin/${target}-ranlib;	export RANLIB
AR=${root}/bin/${target}-ar;		export AR
NM=${root}/bin/${target}-nm;		export NM
SIZE=${root}/bin/${target}-size;	export SIZE
STRIP=${root}/bin/${target}-strip;	export STRIP

STRIPFLAGS="--strip-debug";		export STRIPFLAGS
STRIPPROG=${target}-strip;		export STRIPPROG

HOSTED_CC=cc;				export HOSTED_CC

TARGET=mipsel;				export TARGET
MACHINE=playstation2;			export MACHINE
MACHINE_ARCH=mipsel;			export MACHINE_ARCH

DESTDIR=${bsd_root};			export DESTDIR

MAKE="make";				 export MAKE
#MAKE="make -f /work/cvsrep/sharesrc/share/mk/sys.mk -f Makefile"; export MAKE
#MAKEFLAGS="-I /work/cvsrep/sharesrc/share/mk";	export MAKEFLAGS

set -x
exec $MAKE "$@"
