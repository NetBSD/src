#	$NetBSD: Makefile.inc,v 1.2 1999/01/16 14:59:04 christos Exp $

SRCS+=	rtld_start.S ppc_reloc.c

CPPFLAGS+= -fpic -msoft-float -DELFSIZE=32 -I${.CURDIR}
LDFLAGS+= -Bshareable -Bsymbolic -e .rtld_start
LDFLAGS+= --script ${.CURDIR}/arch/powerpc/ld.so.script
