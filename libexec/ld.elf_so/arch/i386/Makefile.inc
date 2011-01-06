#	$NetBSD: Makefile.inc,v 1.11.2.2 2011/01/06 05:20:25 riz Exp $

SRCS+=		rtld_start.S mdreloc.c

# XXX Should not be in CPPFLAGS!
CPPFLAGS+=	-fpic

CPPFLAGS+=	-DELFSIZE=32

LDFLAGS+=	-Wl,-e,.rtld_start
