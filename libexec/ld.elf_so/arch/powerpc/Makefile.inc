#	$NetBSD: Makefile.inc,v 1.6 2002/09/24 01:59:44 mycroft Exp $

SRCS+=	rtld_start.S ppc_reloc.c

# XXX Should not be in CPPFLAGS!
CPPFLAGS+= -fpic -msoft-float

CPPFLAGS+= -DELFSIZE=32 -DVARPSZ

LDFLAGS+= -Bshareable -Bsymbolic -e .rtld_start
LDFLAGS+= --script ${.CURDIR}/arch/powerpc/ld.so.script
