#	$NetBSD: Makefile.inc,v 1.3 1999/04/18 23:12:59 thorpej Exp $

SRCS+=	rtld_start.S

CPPFLAGS+= -fpic -mno-fp-regs -DELFSIZE=64
LDFLAGS+= -Bshareable -Bsymbolic -e _rtld_start
