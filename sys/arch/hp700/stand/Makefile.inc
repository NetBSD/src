#	$NetBSD: Makefile.inc,v 1.7.94.1 2011/06/23 14:19:12 cherry Exp $

BINDIR=		/usr/mdec

.include <bsd.own.mk>

COPTS+=		-Wno-pointer-sign
COPTS+=		-fno-strict-aliasing
