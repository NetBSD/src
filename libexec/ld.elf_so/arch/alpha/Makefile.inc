#	$NetBSD: Makefile.inc,v 1.2 1997/10/22 05:47:57 lukem Exp $

SRCS+=	rtld_start.S

CPPFLAGS+= -fpic -mno-fp-regs -DELFSIZE=64
LDFLAGS+= -Bshareable -Bsymbolic -e .rtld_start
