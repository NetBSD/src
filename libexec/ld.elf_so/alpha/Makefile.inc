#	$NetBSD: Makefile.inc,v 1.1 1996/12/16 20:38:09 cgd Exp $

SRCS+=	rtld_start.S

CFLAGS+= -fpic -mno-fp-regs -DELFSIZE=64
LDFLAGS+= -Bshareable -Bsymbolic -e .rtld_start
