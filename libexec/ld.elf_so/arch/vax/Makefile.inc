#	$NetBSD: Makefile.inc,v 1.8.2.1 2004/04/29 04:32:58 jmc Exp $

SRCS+=		rtld_start.S mdreloc.c

# XXX Should not be in CPPFLAGS!
CPPFLAGS+=	-fpic

CPPFLAGS+=	-DELFSIZE=32

LDFLAGS+=	-Bsymbolic -e .rtld_start
