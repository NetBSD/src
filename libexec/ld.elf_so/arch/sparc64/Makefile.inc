#	$NetBSD: Makefile.inc,v 1.1 2000/07/13 23:14:17 eeh Exp $

SRCS+=	rtld_start.S mdreloc.c

CPPFLAGS+= -fpic -DELFSIZE=64 -DRTLD_RELOCATE_SELF -DVARPSZ
LDFLAGS+= -Bshareable -Bsymbolic -e _rtld_start
