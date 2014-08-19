#	$NetBSD: Makefile.inc,v 1.19.12.1 2014/08/20 00:02:22 tls Exp $

SRCS+=		rtld_start.S mips_reloc.c

COPTS+=		-G0

CPPFLAGS+=	-DELFSIZE=_MIPS_SZPTR
CPPFLAGS+=	-DRTLD_INHIBIT_COPY_RELOCS
AFLAGS+=	-Wa,--fatal-warnings

LDFLAGS+=	-Wl,-e,rtld_start
