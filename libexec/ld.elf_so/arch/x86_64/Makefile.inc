#	$NetBSD: Makefile.inc,v 1.1 2001/06/19 01:11:44 fvdl Exp $

SRCS+=	rtld_start.S mdreloc.c

CPPFLAGS+= -fpic -DELFSIZE=64 -DRTLD_RELOCATE_SELF
LDFLAGS+= -Bshareable -Bsymbolic -e .rtld_start
