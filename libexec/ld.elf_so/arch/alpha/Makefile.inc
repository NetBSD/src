#	$NetBSD: Makefile.inc,v 1.7 2001/12/14 22:21:46 thorpej Exp $

SRCS+=	rtld_start.S alpha_reloc.c

COPTS+=	-fpic -mno-fp-regs

CPPFLAGS+= -DELFSIZE=64
#CPPFLAGS+= -DRTLD_DEBUG_ALPHA

LDFLAGS+= -Bshareable -Bsymbolic -e _rtld_start
LDFLAGS+= --script ${.CURDIR}/arch/alpha/ld.so.script
