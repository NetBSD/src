#	$NetBSD: Makefile.inc,v 1.3 2001/12/20 19:20:04 thorpej Exp $

SRCS+=	rtld_start.S mdreloc.c

# XXX Should not be in CPPFLAGS!
CPPFLAGS+= -fpic

CPPFLAGS+= -DELFSIZE=64 -DRTLD_RELOCATE_SELF

LDFLAGS+= -Bshareable -Bsymbolic -e .rtld_start
