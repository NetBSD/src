#	$NetBSD: Makefile.inc,v 1.2 1997/04/16 20:29:02 thorpej Exp $

BINDIR=		/usr/mdec

CPPFLAGS+=	-DSTANDALONE

CFLAGS+=	-msoft-float
