#	$NetBSD: Makefile.inc,v 1.4 2002/09/05 15:38:24 mycroft Exp $

SRCS+=	rtld_start.S mdreloc.c

# XXX Should not be in CPPFLAGS!
CPPFLAGS+= -fpic

CPPFLAGS+= -DELFSIZE=32 -DRTLD_RELOCATE_SELF -DVARPSZ

LDFLAGS+= -Bshareable -Bsymbolic -e _rtld_start
