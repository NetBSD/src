#	$NetBSD: Makefile.inc,v 1.2 2000/07/14 22:03:02 matt Exp $

SRCS+=	rtld_start.S

CPPFLAGS+= -fpic -DELFSIZE=32 #-DRTLD_RELOCATE_SELF
LDFLAGS+= -Bshareable -Bsymbolic -e .rtld_start
