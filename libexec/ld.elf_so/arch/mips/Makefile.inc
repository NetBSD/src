#	$NetBSD: Makefile.inc,v 1.12 2002/09/24 01:59:44 mycroft Exp $

SRCS+=	rtld_start.S mips_reloc.c

# XXX Should not be in CPPFLAGS!
CPPFLAGS+= -mabicalls -G0 -fPIC

CPPFLAGS+= -DELFSIZE=32
CPPFLAGS+= -DRTLD_INHIBIT_COPY_RELOCS

LDFLAGS+= -Bshareable -Bsymbolic -e .rtld_start
