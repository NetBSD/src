#	$NetBSD: Makefile.inc,v 1.1 1998/11/24 11:34:31 tsubai Exp $

SRCS+=	rtld_start.S ppc_reloc.c

CPPFLAGS+= -fpic -msoft-float -DELFSIZE=32 -I${.CURDIR}
LDFLAGS+= -Bshareable -Bsymbolic -e .rtld_start
LDFLAGS+= --script ${.CURDIR}/powerpc/ld.so.script
