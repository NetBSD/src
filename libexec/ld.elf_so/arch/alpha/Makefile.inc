#	$NetBSD: Makefile.inc,v 1.11 2004/02/21 15:56:03 skrll Exp $

SRCS+=		rtld_start.S alpha_reloc.c

# XXX Should not be in CPPFLAGS!
CPPFLAGS+=	-fpic -mno-fp-regs

CPPFLAGS+=	-DELFSIZE=64
#CPPFLAGS+=	-DRTLD_DEBUG_ALPHA

LDFLAGS+=	-e _rtld_start
