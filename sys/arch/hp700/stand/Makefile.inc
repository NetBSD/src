#	$NetBSD: Makefile.inc,v 1.9 2013/07/16 10:05:38 joerg Exp $

BINDIR=		/usr/mdec

.include <bsd.own.mk>

COPTS+=		-Wno-pointer-sign
COPTS+=		-fno-strict-aliasing
COPTS+=		-fno-unwind-tables
