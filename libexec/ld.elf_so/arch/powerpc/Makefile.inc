#	$NetBSD: Makefile.inc,v 1.9 2004/02/21 15:56:03 skrll Exp $

SRCS+=		rtld_start.S ppc_reloc.c

# XXX Should not be in CPPFLAGS!
CPPFLAGS+=	-fpic -msoft-float

CPPFLAGS+=	-DELFSIZE=32

LDFLAGS+=	-e .rtld_start
LDFLAGS+=	--script ${.CURDIR}/arch/powerpc/ld.so.script
