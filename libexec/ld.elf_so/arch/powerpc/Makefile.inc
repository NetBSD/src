#	$NetBSD: Makefile.inc,v 1.8 2004/02/20 20:58:24 skrll Exp $

SRCS+=		rtld_start.S ppc_reloc.c

# XXX Should not be in CPPFLAGS!
CPPFLAGS+=	-fpic -msoft-float

CPPFLAGS+=	-DELFSIZE=32

LDFLAGS+=	-Bshareable -Bsymbolic -e .rtld_start
LDFLAGS+=	--script ${.CURDIR}/arch/powerpc/ld.so.script
