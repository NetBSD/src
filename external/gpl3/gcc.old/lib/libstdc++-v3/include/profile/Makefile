#	$NetBSD: Makefile,v 1.9 2025/09/07 04:12:27 mrg Exp $

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
