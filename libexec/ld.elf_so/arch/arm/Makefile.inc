#	$NetBSD: Makefile.inc,v 1.10 2004/02/21 15:56:03 skrll Exp $

SRCS+=		rtld_start.S mdreloc.c

# XXX Should not be in CPPFLAGS!
CPPFLAGS+=	-fpic

CPPFLAGS+=	-DELFSIZE=32

LDFLAGS+=	-e _rtld_start
