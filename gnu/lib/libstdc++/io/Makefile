#	$NetBSD: Makefile,v 1.10 2003/07/22 02:52:38 lukem Exp $

NOOBJ=	# defined

.include <bsd.own.mk>

.include "${.CURDIR}/../arch/${MACHINE_ARCH}/defs.mk"

.cc: # disable .cc->NULL transform

DIST=		${NETBSDSRCDIR}/gnu/dist/toolchain

INCS=		${LIBIO_HEADERS:Niostdio.h}
INCSDIR=	/usr/include/g++

.PATH: ${DIST}/libstdc++ ${DIST}/libstdc++/stl ${DIST}/libio

# Get default targets including <bsd.inc.mk>.
.include <bsd.prog.mk>
