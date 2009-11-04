#	$NetBSD: Makefile.inc,v 1.16 2009/11/04 17:02:43 skrll Exp $

SRCS+=		rtld_start.S mips_reloc.c

# XXX Should not be in CPPFLAGS!
CPPFLAGS+=	-mabicalls -G0 -fPIC

CPPFLAGS+=	-DELFSIZE=32
CPPFLAGS+=	-DRTLD_INHIBIT_COPY_RELOCS

LDFLAGS+=	-Wl,-e,rtld_start
