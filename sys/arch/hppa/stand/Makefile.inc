#	$NetBSD: Makefile.inc,v 1.5 2021/04/13 08:55:06 mrg Exp $

NOSSP=yes
NOPIE=yes
NOCTF=yes
BINDIR=		/usr/mdec

.include <bsd.init.mk>

COPTS+=		-Wno-pointer-sign
COPTS+=		-fno-strict-aliasing
COPTS+=		-fno-unwind-tables
COPTS+=		-fcommon
