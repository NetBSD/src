#	$NetBSD: Makefile.inc,v 1.3 1999/04/18 19:40:07 ws Exp $

SRCS+=	rtld_start.S ppc_reloc.c

CPPFLAGS+= -fpic -msoft-float -DELFSIZE=32 -DVARPSZ -I${.CURDIR}
LDFLAGS+= -Bshareable -Bsymbolic -e .rtld_start
LDFLAGS+= --script ${.CURDIR}/arch/powerpc/ld.so.script
