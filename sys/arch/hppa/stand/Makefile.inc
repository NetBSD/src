#	$NetBSD: Makefile.inc,v 1.1 2014/02/24 07:23:43 skrll Exp $

BINDIR=		/usr/mdec

.include <bsd.own.mk>

COPTS+=		-Wno-pointer-sign
COPTS+=		-fno-strict-aliasing
COPTS+=		-fno-unwind-tables
