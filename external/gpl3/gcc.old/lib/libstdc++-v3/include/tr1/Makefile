#	$NetBSD: Makefile,v 1.5.4.1 2019/06/10 21:58:08 christos Exp $

.include <bsd.own.mk>
.include <bsd.init.mk>

.include "${.CURDIR}/../../arch/${GCC_MACHINE_ARCH}/defs.mk"

.cc: # disable .cc->NULL transform

INCS=		${G_tr1_headers}
INCSDIR=	/usr/include/g++/tr1

.include "${.CURDIR}/../Makefile.includes"

# Get default targets including <bsd.inc.mk>.
.include <bsd.prog.mk>

.PATH: ${DIST}/libstdc++-v3/include
