#	$NetBSD: Makefile.inc,v 1.1 2002/02/03 23:34:43 thorpej Exp $

SRCS+=	rtld_start.S

CPPFLAGS+= -fpic -DELFSIZE=32 -DVARPSZ
LDFLAGS+= -Bshareable -Bsymbolic -e .rtld_start
