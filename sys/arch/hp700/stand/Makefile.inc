#	$NetBSD: Makefile.inc,v 1.8 2011/06/20 06:52:37 mrg Exp $

BINDIR=		/usr/mdec

.include <bsd.own.mk>

COPTS+=		-Wno-pointer-sign
COPTS+=		-fno-strict-aliasing
