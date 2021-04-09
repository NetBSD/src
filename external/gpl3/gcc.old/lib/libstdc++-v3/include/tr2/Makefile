#	$NetBSD: Makefile,v 1.7 2021/04/09 23:15:51 mrg Exp $

.include <bsd.init.mk>

.include "${.CURDIR}/../../arch/${GCC_MACHINE_ARCH}/defs.mk"

.cc: # disable .cc->NULL transform

DIST=		${GCCDIST}
GNUHOSTDIST=	${DIST}

INCS=		${G_tr2_headers}
INCSDIR=	/usr/include/g++/tr2

.include "${.CURDIR}/../Makefile.includes"
