#	$NetBSD: Makefile.inc,v 1.4.2.1 2006/12/30 20:46:01 yamt Exp $

BINDIR=		/usr/mdec

.include <bsd.own.mk>

.if ${HAVE_GCC} > 3
COPTS+=		-Wno-pointer-sign
.endif
COPTS+=		-fno-strict-aliasing
