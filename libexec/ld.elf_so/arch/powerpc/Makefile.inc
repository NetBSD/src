#	$NetBSD: Makefile.inc,v 1.5 2001/12/20 19:20:03 thorpej Exp $

SRCS+=	rtld_start.S ppc_reloc.c

# XXX Should not be in CPPFLAGS!
CPPFLAGS+= -fpic -msoft-float

CPPFLAGS+= -DELFSIZE=32 -DVARPSZ -I${.CURDIR}

LDFLAGS+= -Bshareable -Bsymbolic -e .rtld_start
LDFLAGS+= --script ${.CURDIR}/arch/powerpc/ld.so.script
