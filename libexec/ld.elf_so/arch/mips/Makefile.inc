#	$NetBSD: Makefile.inc,v 1.10 2002/09/12 17:08:32 mycroft Exp $

SRCS+=	rtld_start.S mips_reloc.c

# XXX Should not be in CPPFLAGS!
CPPFLAGS+= -mabicalls -G0 -fPIC

CPPFLAGS+= -DELFSIZE=32 -I${.CURDIR}
CPPFLAGS+= -DRTLD_INHIBIT_COPY_RELOCS

LDFLAGS+= -Bshareable -Bsymbolic -e .rtld_start
LDFLAGS+= --script ${.CURDIR}/arch/mips/ld.so.script
.if (${MACHINE_ARCH} == "mipseb")
LDFLAGS+= -EB
.endif
