#	$NetBSD: Makefile,v 1.10.2.1 2026/11/04 23:00:52 perseant Exp $

.include <bsd.own.mk>
.include <bsd.init.mk>

.include "${.CURDIR}/../../arch/${GCC_MACHINE_ARCH}/defs.mk"

.cc: # disable .cc->NULL transform

INCS=		${G_debug_headers}
INCSDIR=	/usr/include/g++/debug

.include "${.CURDIR}/../Makefile.includes"
