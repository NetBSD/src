#	$NetBSD: Makefile,v 1.1 2021/08/16 06:56:27 mrg Exp $

SUBDIR=		bits

.include <bsd.own.mk>
.include <bsd.init.mk>

.include "${.CURDIR}/../../arch/${GCC_MACHINE_ARCH}/defs.mk"

.cc: # disable .cc->NULL transform

INCS=		${G_experimental_headers}
INCSDIR=	/usr/include/g++/experimental

.include "${.CURDIR}/../Makefile.includes"

.include <bsd.subdir.mk>
