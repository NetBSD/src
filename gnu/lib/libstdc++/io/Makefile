#	$NetBSD: Makefile,v 1.11 2003/10/28 08:12:32 lukem Exp $

NOOBJ=	# defined

.include <bsd.own.mk>

.include "${.CURDIR}/../arch/${MACHINE_ARCH}/defs.mk"

DIST=		${NETBSDSRCDIR}/gnu/dist/toolchain

INCS=		${LIBIO_HEADERS:Niostdio.h}
INCSDIR=	/usr/include/g++

.PATH: ${DIST}/libstdc++ ${DIST}/libstdc++/stl ${DIST}/libio

# Get default targets including <bsd.inc.mk>.
.include <bsd.prog.mk>
