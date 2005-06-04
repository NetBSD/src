#	$NetBSD: Makefile.inc,v 1.10 2005/06/04 16:17:17 lukem Exp $

SRCS+=		rtld_start.S ppc_reloc.c

# XXX Should not be in CPPFLAGS!
CPPFLAGS+=	-fpic -msoft-float

CPPFLAGS+=	-DELFSIZE=32

LDFLAGS+=	-Wl,-e,.rtld_start
LDFLAGS+=	-Wl,--script,${.CURDIR}/arch/powerpc/ld.so.script
