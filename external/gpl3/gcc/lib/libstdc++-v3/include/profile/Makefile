#	$NetBSD: Makefile,v 1.2.4.2 2014/08/19 23:56:56 tls Exp $

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
