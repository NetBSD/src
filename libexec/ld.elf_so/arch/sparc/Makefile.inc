#	$NetBSD: Makefile.inc,v 1.1 1999/02/24 18:25:40 christos Exp $

SRCS+=	rtld_start.S mdreloc.c

CPPFLAGS+= -fpic -DELFSIZE=32 -DRTLD_RELOCATE_SELF
LDFLAGS+= -Bshareable -Bsymbolic -e _rtld_start
