#	$NetBSD: Makefile,v 1.1 2003/07/25 16:32:51 mrg Exp $

.include <bsd.own.mk>

.include "${.CURDIR}/../../arch/${MACHINE_ARCH}/defs.mk"

.cc: # disable .cc->NULL transform

DIST=		${NETBSDSRCDIR}/gnu/dist/gcc

INCS=		${G_ext_headers}
INCSDIR=	/usr/include/g++/ext

.PATH: ${DIST}/libstdc++-v3/include/ext

# Get default targets including <bsd.inc.mk>.
.include <bsd.prog.mk>
