#	$NetBSD: Makefile.inc,v 1.2 2016/02/21 03:33:18 christos Exp $

NOPIE=yes
NOCTF=yes
NODEBUG=yes
BINDIR=		/usr/mdec

.include <bsd.init.mk>

COPTS+=		-Wno-pointer-sign
COPTS+=		-fno-strict-aliasing
COPTS+=		-fno-unwind-tables
