#	$NetBSD: Makefile.inc,v 1.2 2002/09/05 15:38:29 mycroft Exp $

SRCS+=	rtld_start.S mdreloc.c

CPPFLAGS+= -fpic -DELFSIZE=32 -DVARPSZ
LDFLAGS+= -Bshareable -Bsymbolic -e .rtld_start
