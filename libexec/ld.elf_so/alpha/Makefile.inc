#	$NetBSD: Makefile.inc,v 1.1.2.1 1997/11/08 07:25:45 lukem Exp $

SRCS+=	rtld_start.S

CPPFLAGS+= -fpic -mno-fp-regs -DELFSIZE=64
LDFLAGS+= -Bshareable -Bsymbolic -e .rtld_start
