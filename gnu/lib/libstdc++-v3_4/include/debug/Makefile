#	$NetBSD: Makefile,v 1.1.32.1 2013/12/18 18:41:40 matt Exp $

.include <bsd.own.mk>

.include "${.CURDIR}/../../../Makefile.inc"
.include "${.CURDIR}/../../arch/${GCC_MACHINE_ARCH}/defs.mk"

.cc: # disable .cc->NULL transform

DIST=		${NETBSDSRCDIR}/gnu/dist/gcc4

INCS=		${G_debug_headers}
INCSDIR=	/usr/include/g++/debug

.PATH: ${DIST}/libstdc++-v3/include/debug

# Get default targets including <bsd.inc.mk>.
.include <bsd.prog.mk>
