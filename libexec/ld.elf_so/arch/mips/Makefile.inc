#	$NetBSD: Makefile.inc,v 1.15 2005/06/04 16:17:17 lukem Exp $

SRCS+=		rtld_start.S mips_reloc.c

# XXX Should not be in CPPFLAGS!
CPPFLAGS+=	-mabicalls -G0 -fPIC

CPPFLAGS+=	-DELFSIZE=32
CPPFLAGS+=	-DRTLD_INHIBIT_COPY_RELOCS

LDFLAGS+=	-Wl,-e,.rtld_start
