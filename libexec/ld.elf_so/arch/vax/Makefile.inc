#	$NetBSD: Makefile.inc,v 1.4 2001/12/20 19:20:03 thorpej Exp $

SRCS+=	rtld_start.S

# XXX Should not be in CPPFLAGS!
CPPFLAGS+= -fpic

CPPFLAGS+= -DELFSIZE=32 #-DRTLD_RELOCATE_SELF

LDFLAGS+= -Bshareable -Bsymbolic -e .rtld_start
