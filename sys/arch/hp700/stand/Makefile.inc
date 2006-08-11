#	$NetBSD: Makefile.inc,v 1.5.8.2 2006/08/11 15:41:43 yamt Exp $

BINDIR=		/usr/mdec

.include <bsd.own.mk>

.if ${HAVE_GCC} > 3
COPTS+=		-Wno-pointer-sign
.endif
COPTS+=		-fno-strict-aliasing
