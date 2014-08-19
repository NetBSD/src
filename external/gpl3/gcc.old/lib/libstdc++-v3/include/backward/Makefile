#	$NetBSD: Makefile,v 1.1.1.2.8.2 2014/08/19 23:58:22 tls Exp $

.include <bsd.init.mk>

.include "${.CURDIR}/../../arch/${GCC_MACHINE_ARCH}/defs.mk"

.cc: # disable .cc->NULL transform

INCS=		${G_backward_headers}
INCSDIR=	/usr/include/g++/backward

# Get default targets including <bsd.inc.mk>.
.include <bsd.prog.mk>

.PATH: ${DIST}/libstdc++-v3/include/backward
