#	$NetBSD: Makefile.inc,v 1.5.2.1 2004/05/28 08:31:22 tron Exp $

SRCS+=		rtld_start.S ppc_reloc.c

# XXX Should not be in CPPFLAGS!
CPPFLAGS+=	-fpic -msoft-float

CPPFLAGS+=	-DELFSIZE=32

LDFLAGS+=	-Bshareable -Bsymbolic -e .rtld_start
LDFLAGS+=	--script ${.CURDIR}/arch/powerpc/ld.so.script
