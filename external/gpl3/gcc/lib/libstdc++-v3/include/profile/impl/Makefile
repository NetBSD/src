#	$NetBSD: Makefile,v 1.1 2020/09/09 07:23:46 mrg Exp $

.include <bsd.init.mk>
.include "../../Makefile.inc"

.include "${.CURDIR}/../../../arch/${GCC_MACHINE_ARCH}/defs.mk"

.cc: # disable .cc->NULL transform

DIST=		${GCCDIST}
GNUHOSTDIST=	${DIST}

INCS=		${G_profile_impl_headers}
INCSDIR=	/usr/include/g++/profile/impl

.PATH: ${DIST}/libstdc++-v3/include/profile/impl

.include "../../Makefile.includes"
