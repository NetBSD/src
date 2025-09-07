#	$NetBSD: Makefile,v 1.11 2025/09/07 04:12:26 mrg Exp $

.include <bsd.own.mk>
.include <bsd.init.mk>

.include "${.CURDIR}/../../arch/${GCC_MACHINE_ARCH}/defs.mk"

.cc: # disable .cc->NULL transform

INCS=		${G_decimal_headers}
INCSDIR=	/usr/include/g++/decimal

.include "${.CURDIR}/../Makefile.includes"
