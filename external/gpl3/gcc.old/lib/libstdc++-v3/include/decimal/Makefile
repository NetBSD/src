#	$NetBSD: Makefile,v 1.1.1.2.8.2 2014/08/19 23:58:23 tls Exp $

.include <bsd.own.mk>
.include <bsd.init.mk>

.include "${.CURDIR}/../../arch/${GCC_MACHINE_ARCH}/defs.mk"

.cc: # disable .cc->NULL transform

INCS=		${G_decimal_headers}
INCSDIR=	/usr/include/g++/decimal

# Get default targets including <bsd.inc.mk>.
.include <bsd.prog.mk>

.PATH: ${DIST}/libstdc++-v3/include/decimal
