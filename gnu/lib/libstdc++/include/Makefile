#	$NetBSD: Makefile,v 1.5 2003/10/28 08:13:39 lukem Exp $

.include <bsd.own.mk>

.include "${.CURDIR}/../arch/${MACHINE_ARCH}/defs.mk"

DIST=		${NETBSDSRCDIR}/gnu/dist/toolchain

INCS=		${G_HEADERS} ${STD_HEADERS} ${STL_HEADERS}
INCSDIR=	/usr/include/g++

.PATH: ${DIST}/libstdc++ ${DIST}/libstdc++/stl

# Get default targets including <bsd.inc.mk>.
.include <bsd.prog.mk>

.cc: # disable .cc->NULL transform
