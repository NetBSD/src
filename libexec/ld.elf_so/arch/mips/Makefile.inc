#	$NetBSD: Makefile.inc,v 1.11 2002/09/12 18:33:39 mycroft Exp $

SRCS+=	rtld_start.S mips_reloc.c

# XXX Should not be in CPPFLAGS!
CPPFLAGS+= -mabicalls -G0 -fPIC

CPPFLAGS+= -DELFSIZE=32 -I${.CURDIR}
CPPFLAGS+= -DRTLD_INHIBIT_COPY_RELOCS

LDFLAGS+= -Bshareable -Bsymbolic -e .rtld_start
