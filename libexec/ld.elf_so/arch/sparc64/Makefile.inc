#	$NetBSD: Makefile.inc,v 1.2 2001/12/14 22:21:47 thorpej Exp $

SRCS+=	rtld_start.S mdreloc.c

COPTS+=	-fpic

CPPFLAGS+= -DELFSIZE=64 -DRTLD_RELOCATE_SELF -DVARPSZ

LDFLAGS+= -Bshareable -Bsymbolic -e _rtld_start
