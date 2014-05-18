#	$NetBSD: Makefile.inc,v 1.1.4.2 2014/05/18 17:45:11 rmind Exp $

BINDIR=		/usr/mdec

.include <bsd.own.mk>

COPTS+=		-Wno-pointer-sign
COPTS+=		-fno-strict-aliasing
COPTS+=		-fno-unwind-tables
