#	$NetBSD: Makefile,v 1.9 2021/04/09 23:15:48 mrg Exp $

.include <bsd.init.mk>

.include "${.CURDIR}/../../arch/${GCC_MACHINE_ARCH}/defs.mk"

.cc: # disable .cc->NULL transform

INCS=		${G_backward_headers}
INCSDIR=	/usr/include/g++/backward

.include "${.CURDIR}/../Makefile.includes"
