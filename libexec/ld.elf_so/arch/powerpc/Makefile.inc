#	$NetBSD: Makefile.inc,v 1.4 2001/12/14 22:21:47 thorpej Exp $

SRCS+=	rtld_start.S ppc_reloc.c

COPTS+=	-fpic -msoft-float

CPPFLAGS+= -DELFSIZE=32 -DVARPSZ -I${.CURDIR}

LDFLAGS+= -Bshareable -Bsymbolic -e .rtld_start
LDFLAGS+= --script ${.CURDIR}/arch/powerpc/ld.so.script
