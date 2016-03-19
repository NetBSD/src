#	$NetBSD: Makefile.inc,v 1.1.12.1 2016/03/19 11:30:00 skrll Exp $

NOPIE=yes
NOCTF=yes
BINDIR=		/usr/mdec

.include <bsd.init.mk>

COPTS+=		-Wno-pointer-sign
COPTS+=		-fno-strict-aliasing
COPTS+=		-fno-unwind-tables
