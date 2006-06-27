#	$NetBSD: Makefile.inc,v 1.7 2006/06/27 06:37:26 he Exp $

BINDIR=		/usr/mdec

.include <bsd.own.mk>

.if ${HAVE_GCC} > 3
COPTS+=		-Wno-pointer-sign
.endif
COPTS+=		-fno-strict-aliasing
