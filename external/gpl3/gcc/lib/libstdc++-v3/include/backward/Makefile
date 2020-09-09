#	$NetBSD: Makefile,v 1.7 2020/09/09 07:23:45 mrg Exp $

.include <bsd.init.mk>

.include "${.CURDIR}/../../arch/${GCC_MACHINE_ARCH}/defs.mk"

.cc: # disable .cc->NULL transform

INCS=		${G_backward_headers}
INCSDIR=	/usr/include/g++/backward

.include "${.CURDIR}/../Makefile.includes"
