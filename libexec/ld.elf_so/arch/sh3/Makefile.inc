#	$NetBSD: Makefile.inc,v 1.4 2004/02/20 20:58:24 skrll Exp $

SRCS+=		rtld_start.S mdreloc.c

CPPFLAGS+=	-fpic -DELFSIZE=32
LDFLAGS+=	-Bshareable -Bsymbolic -e .rtld_start
