#	$NetBSD: Makefile.inc,v 1.9 2004/04/28 02:04:56 mhitch Exp $

SRCS+=		rtld_start.S mdreloc.c

# XXX Should not be in CPPFLAGS!
CPPFLAGS+=	-fpic

CPPFLAGS+=	-DELFSIZE=32

LDFLAGS+=	-Bsymbolic -e .rtld_start
