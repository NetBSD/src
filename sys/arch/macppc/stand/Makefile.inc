#	$NetBSD: Makefile.inc,v 1.2.8.1 2000/07/13 21:01:11 he Exp $

BINDIR=		/usr/mdec

CPPFLAGS+=	-D_STANDALONE

CFLAGS+=	-msoft-float

.NOTMAIN: release

release:
