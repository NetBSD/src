#	$NetBSD: Makefile,v 1.2 2023/02/20 02:12:27 mrg Exp $

SUBDIR=		bits

.include <bsd.own.mk>
.include <bsd.init.mk>

.include "${.CURDIR}/../../arch/${GCC_MACHINE_ARCH}/defs.mk"

.cc: # disable .cc->NULL transform

INCS=		${G_experimental_headers}
INCSDIR=	/usr/include/g++/experimental

.include "${.CURDIR}/../Makefile.includes"

.include <bsd.subdir.mk>
