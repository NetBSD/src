#	$NetBSD: Makefile.inc,v 1.6 2004/02/21 15:56:03 skrll Exp $

SRCS+=		rtld_start.S mdreloc.c

# XXX Should not be in CPPFLAGS!
CPPFLAGS+=	-fPIC

CPPFLAGS+=	-DELFSIZE=64

LDFLAGS+=	-e .rtld_start
