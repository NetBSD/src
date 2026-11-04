#	$NetBSD: Makefile,v 1.8.2.1 2026/11/04 23:00:53 perseant Exp $

.include <bsd.init.mk>

.include "${.CURDIR}/../../arch/${GCC_MACHINE_ARCH}/defs.mk"

.cc: # disable .cc->NULL transform

DIST=		${GCCDIST}
GNUHOSTDIST=	${DIST}

INCS=		${G_profile_headers}
INCSDIR=	/usr/include/g++/profile

.PATH: ${DIST}/libstdc++-v3/include/profile

.include "${.CURDIR}/../Makefile.includes"

SUBDIR=	impl

.include <bsd.subdir.mk>
