#	$NetBSD: Makefile.inc,v 1.1 1999/04/29 15:06:42 kleink Exp $

SRCS+=	rtld_start.S

CPPFLAGS+= -fpic -DELFSIZE=32 -DRTLD_RELOCATE_SELF
LDFLAGS+= -Bshareable -Bsymbolic -e .rtld_start
