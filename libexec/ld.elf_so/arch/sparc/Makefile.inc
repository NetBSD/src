#	$NetBSD: Makefile.inc,v 1.7 2002/09/24 12:12:10 mycroft Exp $

SRCS+=	rtld_start.S mdreloc.c

# XXX Should not be in CPPFLAGS!
CPPFLAGS+= -fpic

CPPFLAGS+= -DELFSIZE=32

LDFLAGS+= -Bshareable -Bsymbolic -e _rtld_start
