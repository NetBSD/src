#	$NetBSD: Makefile.inc,v 1.5 2004/02/20 20:58:25 skrll Exp $

SRCS+=		rtld_start.S mdreloc.c

# XXX Should not be in CPPFLAGS!
CPPFLAGS+=	-fPIC

CPPFLAGS+=	-DELFSIZE=64

LDFLAGS+=	-Bshareable -Bsymbolic -e .rtld_start
