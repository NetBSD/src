#	$NetBSD: Makefile,v 1.1 2006/05/10 22:53:50 mrg Exp $

.include <bsd.own.mk>

.include "${.CURDIR}/../../arch/${MACHINE_ARCH}/defs.mk"

.cc: # disable .cc->NULL transform

DIST=		${NETBSDSRCDIR}/gnu/dist/gcc4

INCS=		${G_debug_headers}
INCSDIR=	/usr/include/g++/debug

.PATH: ${DIST}/libstdc++-v3/include/debug

# Get default targets including <bsd.inc.mk>.
.include <bsd.prog.mk>
