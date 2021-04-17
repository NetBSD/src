#	$NetBSD: Makefile.inc,v 1.4.18.1 2021/04/17 17:26:13 thorpej Exp $

NOSSP=yes
NOPIE=yes
NOCTF=yes
BINDIR=		/usr/mdec

.include <bsd.init.mk>

COPTS+=		-Wno-pointer-sign
COPTS+=		-fno-strict-aliasing
COPTS+=		-fno-unwind-tables
COPTS+=		-fcommon
