#	$NetBSD: Makefile.inc,v 1.3 2002/09/24 12:12:09 mycroft Exp $

SRCS+=	rtld_start.S mdreloc.c

CPPFLAGS+= -fpic -DELFSIZE=32
LDFLAGS+= -Bshareable -Bsymbolic -e .rtld_start
