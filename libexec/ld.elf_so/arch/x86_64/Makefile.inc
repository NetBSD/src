#	$NetBSD: Makefile.inc,v 1.4 2002/11/24 18:16:45 fvdl Exp $

SRCS+=	rtld_start.S mdreloc.c

# XXX Should not be in CPPFLAGS!
CPPFLAGS+= -fPIC

CPPFLAGS+= -DELFSIZE=64

LDFLAGS+= -Bshareable -Bsymbolic -e .rtld_start
