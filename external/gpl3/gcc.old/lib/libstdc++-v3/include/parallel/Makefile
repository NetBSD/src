#	$NetBSD: Makefile,v 1.1.1.3 2015/09/23 03:03:39 mrg Exp $

.include <bsd.own.mk>
.include <bsd.init.mk>

.include "${.CURDIR}/../../arch/${GCC_MACHINE_ARCH}/defs.mk"

.cc: # disable .cc->NULL transform

INCS=		${G_parallel_headers}
INCSDIR=	/usr/include/g++/parallel

.include "${.CURDIR}/../Makefile.includes"

# Get default targets including <bsd.inc.mk>.
.include <bsd.prog.mk>

.PATH: ${DIST}/libstdc++-v3/include
