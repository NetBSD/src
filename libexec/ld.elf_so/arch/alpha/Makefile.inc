#	$NetBSD: Makefile.inc,v 1.5 2001/12/13 22:34:52 thorpej Exp $

SRCS+=	rtld_start.S alpha_reloc.c

CPPFLAGS+= -fpic -mno-fp-regs -DELFSIZE=64
LDFLAGS+= -Bshareable -Bsymbolic -e _rtld_start
LDFLAGS+= --script ${.CURDIR}/arch/alpha/ld.so.script
