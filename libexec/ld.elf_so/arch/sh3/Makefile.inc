#	$NetBSD: Makefile.inc,v 1.5 2004/02/21 15:56:03 skrll Exp $

SRCS+=		rtld_start.S mdreloc.c

CPPFLAGS+=	-fpic -DELFSIZE=32
LDFLAGS+=	-e .rtld_start
