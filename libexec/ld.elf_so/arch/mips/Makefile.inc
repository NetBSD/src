#	$NetBSD: Makefile.inc,v 1.9.2.1 2004/05/28 08:31:22 tron Exp $

SRCS+=		rtld_start.S mips_reloc.c

# XXX Should not be in CPPFLAGS!
CPPFLAGS+=	-mabicalls -G0 -fPIC

CPPFLAGS+=	-DELFSIZE=32
CPPFLAGS+=	-DRTLD_INHIBIT_COPY_RELOCS

LDFLAGS+=	-Bshareable -Bsymbolic -e .rtld_start
