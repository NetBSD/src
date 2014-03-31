#	$NetBSD: Makefile.playstation2.inc,v 1.5 2014/03/31 11:43:53 martin Exp $

.if !defined(TOOLCHAIN_MISSING) || ${TOOLCHAIN_MISSING}=="no"
.error please do not run "make" directly, use sh ../../conf/build.playstation2.sh instead
.endif

# working around makesystem bugs: redefine CC and friends here or it does
# not properly get picked up by mkdep in sub-makes for "make depend" of
# libkern and friends - XXX fix this
PREFIX=/usr/pkg
PLATFORM=mips--netbsdelf

CC=${PREFIX}/bin/${PLATFORM}-gcc
CPP=${PREFIX}/bin/${PLATFORM}-cpp
AS=${PREFIX}/bin/${PLATFORM}-as
AR=${PREFIX}/bin/${PLATFORM}-ar
LD=${PREFIX}/bin/${PLATFORM}-ld
RANLIB=${PREFIX}/bin/${PLATFORM}-ranlib
NM=${PREFIX}/bin/${PLATFORM}-nm
SIZE=${PREFIX}/bin/${PLATFORM}-size
STRIP=${PREFIX}/bin/${PLATFORM}-strip

# Playstation2 uses 128 bit access, which currently requires n32 ABI
CFLAGS+= -mabi=n32 -march=r5900 -fno-pic
AFLAGS+= -mabi=n32 -march=r5900 -fno-pic
