#	$NetBSD: Makefile.inc,v 1.1.10.3 2017/12/03 11:36:16 jdolecek Exp $

NOSSP=yes
NOPIE=yes
NOCTF=yes
BINDIR=		/usr/mdec

.include <bsd.init.mk>

COPTS+=		-Wno-pointer-sign
COPTS+=		-fno-strict-aliasing
COPTS+=		-fno-unwind-tables
