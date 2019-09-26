#	$NetBSD: Makefile,v 1.6 2019/09/26 08:03:58 mrg Exp $

.include <bsd.init.mk>

.include "${.CURDIR}/../../arch/${GCC_MACHINE_ARCH}/defs.mk"

.cc: # disable .cc->NULL transform

DIST=		${GCCDIST}
GNUHOSTDIST=	${DIST}

INCS=		${G_profile_headers} ${G_profile_impl_headers}
INCSDIR=	/usr/include/g++/profile

.PATH: ${DIST}/libstdc++-v3/include/profile

# Get default targets including <bsd.inc.mk>.
.include <bsd.prog.mk>
