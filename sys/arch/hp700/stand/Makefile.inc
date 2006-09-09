#	$NetBSD: Makefile.inc,v 1.5.4.1 2006/09/09 02:39:26 rpaulo Exp $

BINDIR=		/usr/mdec

.include <bsd.own.mk>

.if ${HAVE_GCC} > 3
COPTS+=		-Wno-pointer-sign
.endif
COPTS+=		-fno-strict-aliasing
