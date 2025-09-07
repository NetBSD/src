#	$NetBSD: Makefile,v 1.3 2025/09/07 04:12:26 mrg Exp $

SUBDIR=		bits

.include <bsd.own.mk>
.include <bsd.init.mk>

.include "${.CURDIR}/../../arch/${GCC_MACHINE_ARCH}/defs.mk"

.cc: # disable .cc->NULL transform

INCS=		${G_experimental_headers}
INCSDIR=	/usr/include/g++/experimental

.include "${.CURDIR}/../Makefile.includes"

.include <bsd.subdir.mk>
