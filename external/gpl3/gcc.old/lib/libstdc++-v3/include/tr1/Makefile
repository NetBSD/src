#	$NetBSD: Makefile,v 1.9 2021/04/09 23:15:50 mrg Exp $

.include <bsd.own.mk>
.include <bsd.init.mk>

.include "${.CURDIR}/../../arch/${GCC_MACHINE_ARCH}/defs.mk"

.cc: # disable .cc->NULL transform

INCS=		${G_tr1_headers}
INCSDIR=	/usr/include/g++/tr1

.include "${.CURDIR}/../Makefile.includes"
