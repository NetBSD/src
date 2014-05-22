#	$NetBSD: Makefile,v 1.1.1.2.4.2 2014/05/22 15:58:50 yamt Exp $

.include <bsd.own.mk>
.include <bsd.init.mk>

.include "${.CURDIR}/../../arch/${GCC_MACHINE_ARCH}/defs.mk"

.cc: # disable .cc->NULL transform

INCS=		${G_debug_headers}
INCSDIR=	/usr/include/g++/debug

# Get default targets including <bsd.inc.mk>.
.include <bsd.prog.mk>

.PATH: ${DIST}/libstdc++-v3/include/debug
