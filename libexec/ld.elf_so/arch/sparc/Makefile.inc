#	$NetBSD: Makefile.inc,v 1.6 2002/09/11 19:46:04 mycroft Exp $

SRCS+=	rtld_start.S mdreloc.c

# XXX Should not be in CPPFLAGS!
CPPFLAGS+= -fpic

CPPFLAGS+= -DELFSIZE=32 -DVARPSZ

LDFLAGS+= -Bshareable -Bsymbolic -e _rtld_start
