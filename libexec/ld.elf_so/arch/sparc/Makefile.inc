#	$NetBSD: Makefile.inc,v 1.8 2004/02/20 20:58:24 skrll Exp $

SRCS+=		rtld_start.S mdreloc.c

# XXX Should not be in CPPFLAGS!
CPPFLAGS+=	-fpic

CPPFLAGS+=	-DELFSIZE=32

LDFLAGS+=	-Bshareable -Bsymbolic -e _rtld_start
