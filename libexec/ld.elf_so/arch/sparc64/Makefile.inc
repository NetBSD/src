#	$NetBSD: Makefile.inc,v 1.3 2001/12/20 19:20:03 thorpej Exp $

SRCS+=	rtld_start.S mdreloc.c

# XXX Should not be in CPPFLAGS!
CPPFLAGS+= -fpic

CPPFLAGS+= -DELFSIZE=64 -DRTLD_RELOCATE_SELF -DVARPSZ

LDFLAGS+= -Bshareable -Bsymbolic -e _rtld_start
