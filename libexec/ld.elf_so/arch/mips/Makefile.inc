#	$NetBSD: Makefile.inc,v 1.14 2004/02/21 15:56:03 skrll Exp $

SRCS+=		rtld_start.S mips_reloc.c

# XXX Should not be in CPPFLAGS!
CPPFLAGS+=	-mabicalls -G0 -fPIC

CPPFLAGS+=	-DELFSIZE=32
CPPFLAGS+=	-DRTLD_INHIBIT_COPY_RELOCS

LDFLAGS+=	-e .rtld_start
