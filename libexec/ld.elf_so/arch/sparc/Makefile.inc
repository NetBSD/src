#	$NetBSD: Makefile.inc,v 1.2 1999/06/22 16:28:23 christos Exp $

SRCS+=	rtld_start.S mdreloc.c

CPPFLAGS+= -fpic -DELFSIZE=32 -DRTLD_RELOCATE_SELF -DVARPSZ
LDFLAGS+= -Bshareable -Bsymbolic -e _rtld_start
