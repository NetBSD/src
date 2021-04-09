#	$NetBSD: Makefile,v 1.1.1.4 2021/04/09 22:33:16 mrg Exp $

.include <bsd.own.mk>
.include <bsd.init.mk>

.include "${.CURDIR}/../../arch/${GCC_MACHINE_ARCH}/defs.mk"

.cc: # disable .cc->NULL transform

INCS=		${G_decimal_headers}
INCSDIR=	/usr/include/g++/decimal

.include "${.CURDIR}/../Makefile.includes"
