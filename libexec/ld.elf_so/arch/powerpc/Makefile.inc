#	$NetBSD: Makefile.inc,v 1.7 2002/09/24 12:12:09 mycroft Exp $

SRCS+=	rtld_start.S ppc_reloc.c

# XXX Should not be in CPPFLAGS!
CPPFLAGS+= -fpic -msoft-float

CPPFLAGS+= -DELFSIZE=32

LDFLAGS+= -Bshareable -Bsymbolic -e .rtld_start
LDFLAGS+= --script ${.CURDIR}/arch/powerpc/ld.so.script
