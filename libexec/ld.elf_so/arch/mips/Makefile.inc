#	$NetBSD: Makefile.inc,v 1.6 1999/04/16 15:49:20 simonb Exp $

SRCS+=	rtld_start.S mips_reloc.c

CPPFLAGS+= -mabicalls -G0 -fPIC -DELFSIZE=32 -I${.CURDIR}
CPPFLAGS+= -DRTLD_INHIBIT_COPY_RELOCS

.if (${MACHINE} == "newsmips")
# UGH!  ld.so.script specifies little-endian; need to override that here
LDFLAGS+= -EB
.endif

LDFLAGS+= -Bshareable -Bsymbolic -e .rtld_start
LDFLAGS+= --script ${.CURDIR}/arch/mips/ld.so.script
