#	$NetBSD: Makefile.inc,v 1.6 2001/12/14 00:53:07 thorpej Exp $

SRCS+=	rtld_start.S alpha_reloc.c

CPPFLAGS+= -fpic -mno-fp-regs -DELFSIZE=64
#CPPFLAGS+= -DRTLD_DEBUG_ALPHA
LDFLAGS+= -Bshareable -Bsymbolic -e _rtld_start
LDFLAGS+= --script ${.CURDIR}/arch/alpha/ld.so.script
