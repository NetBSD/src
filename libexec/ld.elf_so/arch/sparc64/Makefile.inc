#	$NetBSD: Makefile.inc,v 1.4 2002/09/11 19:46:40 mycroft Exp $

SRCS+=	rtld_start.S mdreloc.c

# XXX Should not be in CPPFLAGS!
CPPFLAGS+= -fpic

CPPFLAGS+= -DELFSIZE=64 -DVARPSZ

LDFLAGS+= -Bshareable -Bsymbolic -e _rtld_start
