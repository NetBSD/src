#	$NetBSD: Makefile.inc,v 1.1 1999/01/10 18:21:24 christos Exp $

SRCS+=	rtld_start.S

CPPFLAGS+= -fpic -DELFSIZE=32
LDFLAGS+= -Bshareable -Bsymbolic -e .rtld_start
