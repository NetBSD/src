#	$NetBSD: Makefile.inc,v 1.2 1999/02/24 18:31:00 christos Exp $

SRCS+=	rtld_start.S

CPPFLAGS+= -fpic -DELFSIZE=32 -DRTLD_RELOCATE_SELF
LDFLAGS+= -Bshareable -Bsymbolic -e .rtld_start
