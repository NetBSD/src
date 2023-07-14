#	$NetBSD: Makefile,v 1.1.1.1 2023/07/14 06:34:27 mrg Exp $

.include <bsd.init.mk>

.include "${.CURDIR}/../../arch/${GCC_MACHINE_ARCH}/defs.mk"

.cc: # disable .cc->NULL transform

DIST=		${GCCDIST}
GNUHOSTDIST=	${DIST}

INCS=		${G_pstl_headers}
INCSDIR=	/usr/include/g++/pstl

.include "${.CURDIR}/../Makefile.includes"
