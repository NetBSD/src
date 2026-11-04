#	$NetBSD: Makefile,v 1.2.2.1 2026/11/04 23:00:53 perseant Exp $

.include <bsd.init.mk>

.include "${.CURDIR}/../../arch/${GCC_MACHINE_ARCH}/defs.mk"

.cc: # disable .cc->NULL transform

DIST=		${GCCDIST}
GNUHOSTDIST=	${DIST}

INCS=		${G_pstl_headers}
INCSDIR=	/usr/include/g++/pstl

.include "${.CURDIR}/../Makefile.includes"
