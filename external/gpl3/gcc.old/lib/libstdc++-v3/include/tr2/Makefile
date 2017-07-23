#	$NetBSD: Makefile,v 1.3 2017/07/23 01:12:16 mrg Exp $

.include <bsd.init.mk>

.include "${.CURDIR}/../../arch/${GCC_MACHINE_ARCH}/defs.mk"

.cc: # disable .cc->NULL transform

DIST=		${GCCDIST}
GNUHOSTDIST=	${DIST}

INCS=		${G_tr2_headers}
INCSDIR=	/usr/include/g++/tr2

.include "${.CURDIR}/../Makefile.includes"

.PATH: ${DIST}/libstdc++-v3/include

# Get default targets including <bsd.inc.mk>.
.include <bsd.prog.mk>
