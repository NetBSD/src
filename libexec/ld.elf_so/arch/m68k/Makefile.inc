#	$NetBSD: Makefile.inc,v 1.5 2002/09/05 15:38:27 mycroft Exp $

SRCS+=	rtld_start.S mdreloc.c

# XXX Should not be in CPPFLAGS!
CPPFLAGS+= -fpic

CPPFLAGS+= -DELFSIZE=32 -DRTLD_RELOCATE_SELF -DVARPSZ

LDFLAGS+= -Bshareable -Bsymbolic -e .rtld_start
