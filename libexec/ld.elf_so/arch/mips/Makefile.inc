#	$NetBSD: Makefile.inc,v 1.8 2001/12/14 22:21:47 thorpej Exp $

SRCS+=	rtld_start.S mips_reloc.c

COPTS+=	-mabicalls -G0 -fPIC

CPPFLAGS+= -DELFSIZE=32 -I${.CURDIR}
CPPFLAGS+= -DRTLD_INHIBIT_COPY_RELOCS

.if (${MACHINE_ARCH} == "mipseb")
# UGH!  ld.so.script specifies little-endian; need to override that here
LDFLAGS+= -EB
.endif

LDFLAGS+= -Bshareable -Bsymbolic -e .rtld_start
LDFLAGS+= --script ${.CURDIR}/arch/mips/ld.so.script
