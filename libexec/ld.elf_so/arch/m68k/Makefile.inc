#	$NetBSD: Makefile.inc,v 1.6 2002/09/11 20:45:52 mycroft Exp $

SRCS+=	rtld_start.S mdreloc.c

# XXX Should not be in CPPFLAGS!
CPPFLAGS+= -fpic

CPPFLAGS+= -DELFSIZE=32 -DVARPSZ

LDFLAGS+= -Bshareable -Bsymbolic -e .rtld_start
