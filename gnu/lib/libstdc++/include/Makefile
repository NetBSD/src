#	$NetBSD: Makefile,v 1.1.2.1 2004/03/28 07:47:41 jmc Exp $

.include <bsd.own.mk>

.if defined(USE_NEW_TOOLCHAIN)

.include "${.CURDIR}/../arch/${MACHINE_ARCH}/defs.mk"

.cc: # disable .cc->NULL transform

TOP=		${.CURDIR}/../../..
DIST=		${TOP}/dist/toolchain

INCS=		${G_HEADERS} ${STD_HEADERS} ${STL_HEADERS}
INCSDIR=	/usr/include/g++

.PATH: ${DIST}/libstdc++ ${DIST}/libstdc++/stl

# Get default targets including <bsd.inc.mk>.
.include <bsd.prog.mk>

.endif
