#	$NetBSD: Makefile,v 1.1 2001/07/24 19:44:50 tv Exp $

.include <bsd.own.mk>

# This include file keeps the includes/build of libstdc++ separate,
# to avoid issues like "iostream.cc" using the default ".cc:" rule
# to build the "iostream" file (which is actually a header).

.if defined(USE_NEW_TOOLCHAIN)

.include "${.CURDIR}/../arch/${MACHINE_ARCH}/defs.mk"

TOP=		${.CURDIR}/../../..
DIST=		${TOP}/dist/toolchain

INCS=		${G_HEADERS} ${STD_HEADERS} ${STL_HEADERS}
INCSDIR=	/usr/include/g++

.PATH: ${DIST}/libstdc++ ${DIST}/libstdc++/stl

# Get default targets including <bsd.inc.mk>.
.include <bsd.prog.mk>

.endif
