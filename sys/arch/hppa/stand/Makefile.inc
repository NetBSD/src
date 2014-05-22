#	$NetBSD: Makefile.inc,v 1.1.6.2 2014/05/22 11:39:50 yamt Exp $

BINDIR=		/usr/mdec

.include <bsd.own.mk>

COPTS+=		-Wno-pointer-sign
COPTS+=		-fno-strict-aliasing
COPTS+=		-fno-unwind-tables
