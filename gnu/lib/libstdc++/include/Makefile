#	$NetBSD: Makefile,v 1.4 2003/07/22 02:52:37 lukem Exp $

.include <bsd.own.mk>

.include "${.CURDIR}/../arch/${MACHINE_ARCH}/defs.mk"

.cc: # disable .cc->NULL transform

DIST=		${NETBSDSRCDIR}/gnu/dist/toolchain

INCS=		${G_HEADERS} ${STD_HEADERS} ${STL_HEADERS}
INCSDIR=	/usr/include/g++

.PATH: ${DIST}/libstdc++ ${DIST}/libstdc++/stl

# Get default targets including <bsd.inc.mk>.
.include <bsd.prog.mk>
