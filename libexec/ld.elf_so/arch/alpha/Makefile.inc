#	$NetBSD: Makefile.inc,v 1.4 2001/12/13 20:30:48 thorpej Exp $

SRCS+=	rtld_start.S

CPPFLAGS+= -fpic -mno-fp-regs -DELFSIZE=64
LDFLAGS+= -Bshareable -Bsymbolic -e _rtld_start
LDFLAGS+= --script ${.CURDIR}/arch/alpha/ld.so.script
