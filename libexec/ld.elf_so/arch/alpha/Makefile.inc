#	$NetBSD: Makefile.inc,v 1.10 2004/02/20 20:58:24 skrll Exp $

SRCS+=		rtld_start.S alpha_reloc.c

# XXX Should not be in CPPFLAGS!
CPPFLAGS+=	-fpic -mno-fp-regs

CPPFLAGS+=	-DELFSIZE=64
#CPPFLAGS+=	-DRTLD_DEBUG_ALPHA

LDFLAGS+=	-Bshareable -Bsymbolic -e _rtld_start
