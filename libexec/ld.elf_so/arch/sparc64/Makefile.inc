#	$NetBSD: Makefile.inc,v 1.1.2.2 2000/07/26 23:45:23 mycroft Exp $

SRCS+=	rtld_start.S mdreloc.c

CPPFLAGS+= -fpic -DELFSIZE=64 -DRTLD_RELOCATE_SELF -DVARPSZ
LDFLAGS+= -Bshareable -Bsymbolic -e _rtld_start
