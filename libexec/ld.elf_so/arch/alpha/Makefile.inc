#	$NetBSD: Makefile.inc,v 1.9 2002/09/11 18:20:47 mycroft Exp $

SRCS+=	rtld_start.S alpha_reloc.c

# XXX Should not be in CPPFLAGS!
CPPFLAGS+= -fpic -mno-fp-regs

CPPFLAGS+= -DELFSIZE=64
#CPPFLAGS+= -DRTLD_DEBUG_ALPHA

LDFLAGS+= -Bshareable -Bsymbolic -e _rtld_start
