#	$NetBSD: Makefile.inc,v 1.1.10.2 2014/08/20 00:03:05 tls Exp $

BINDIR=		/usr/mdec

.include <bsd.own.mk>

COPTS+=		-Wno-pointer-sign
COPTS+=		-fno-strict-aliasing
COPTS+=		-fno-unwind-tables
