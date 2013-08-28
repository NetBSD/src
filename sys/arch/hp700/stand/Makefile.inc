#	$NetBSD: Makefile.inc,v 1.8.16.1 2013/08/28 23:59:16 rmind Exp $

BINDIR=		/usr/mdec

.include <bsd.own.mk>

COPTS+=		-Wno-pointer-sign
COPTS+=		-fno-strict-aliasing
COPTS+=		-fno-unwind-tables
