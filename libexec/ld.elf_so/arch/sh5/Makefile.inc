#	$NetBSD: Makefile.inc,v 1.1 2003/03/25 13:11:54 scw Exp $

SRCS+=	rtld_start.S mdreloc.c

# XXX Should not be in CPPFLAGS!
CPPFLAGS+= -fpic

CPPFLAGS+= -DELFSIZE=32

LDFLAGS+= -Bshareable -Bsymbolic -e .rtld_start
