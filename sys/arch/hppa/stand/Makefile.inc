#	$NetBSD: Makefile.inc,v 1.3.10.1 2017/08/31 11:41:29 martin Exp $

NOSSP=yes
NOPIE=yes
NOCTF=yes
BINDIR=		/usr/mdec

.include <bsd.init.mk>

COPTS+=		-Wno-pointer-sign
COPTS+=		-fno-strict-aliasing
COPTS+=		-fno-unwind-tables
