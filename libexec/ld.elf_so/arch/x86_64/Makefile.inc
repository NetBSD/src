#	$NetBSD: Makefile.inc,v 1.2 2001/12/14 22:21:48 thorpej Exp $

SRCS+=	rtld_start.S mdreloc.c

COPTS+=	-fpic

CPPFLAGS+= -DELFSIZE=64 -DRTLD_RELOCATE_SELF

LDFLAGS+= -Bshareable -Bsymbolic -e .rtld_start
