#	$NetBSD: Makefile.inc,v 1.1 1999/08/21 19:26:19 matt Exp $

SRCS+=	rtld_start.S

CPPFLAGS+= -fpic -DELFSIZE=32 -DRTLD_RELOCATE_SELF
LDFLAGS+= -Bshareable -Bsymbolic -e .rtld_start
