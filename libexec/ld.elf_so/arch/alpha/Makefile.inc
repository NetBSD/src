#	$NetBSD: Makefile.inc,v 1.8.2.1 2004/05/28 08:31:22 tron Exp $

SRCS+=		rtld_start.S alpha_reloc.c

# XXX Should not be in CPPFLAGS!
CPPFLAGS+=	-fpic -mno-fp-regs

CPPFLAGS+=	-DELFSIZE=64
#CPPFLAGS+=	-DRTLD_DEBUG_ALPHA

LDFLAGS+=	-Bshareable -Bsymbolic -e _rtld_start
