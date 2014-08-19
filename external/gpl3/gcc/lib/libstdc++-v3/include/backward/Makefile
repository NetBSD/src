#	$NetBSD: Makefile,v 1.2.8.1 2014/08/19 23:56:55 tls Exp $

.include <bsd.init.mk>

.include "${.CURDIR}/../../arch/${GCC_MACHINE_ARCH}/defs.mk"

.cc: # disable .cc->NULL transform

INCS=		${G_backward_headers}
INCSDIR=	/usr/include/g++/backward

.include "${.CURDIR}/../Makefile.includes"

# Get default targets including <bsd.inc.mk>.
.include <bsd.prog.mk>

.PATH: ${DIST}/libstdc++-v3/include
