#	$NetBSD: Makefile.inc,v 1.5.16.1 2006/07/13 17:48:49 gdamore Exp $

BINDIR=		/usr/mdec

.include <bsd.own.mk>

.if ${HAVE_GCC} > 3
COPTS+=		-Wno-pointer-sign
.endif
COPTS+=		-fno-strict-aliasing
