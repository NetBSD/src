#	$NetBSD: Makefile.inc,v 1.7 2002/09/11 17:23:23 mycroft Exp $

SRCS+=	rtld_start.S mdreloc.c

# XXX Should not be in CPPFLAGS!
CPPFLAGS+= -fpic

CPPFLAGS+= -DELFSIZE=32

LDFLAGS+= -Bshareable -Bsymbolic -e .rtld_start
