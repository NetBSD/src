#	$NetBSD: Makefile.inc,v 1.15.32.1 2009/08/26 03:32:53 matt Exp $

SRCS+=		rtld_start.S mips_reloc.c

# XXX Should not be in CPPFLAGS!
CPUFLAGS+=	-G0

CPPFLAGS+=	-DELFSIZE=32
CPPFLAGS+=	-DRTLD_INHIBIT_COPY_RELOCS
AFLAGS+=	-Wa,--fatal-warnings

LDFLAGS+=	-Wl,-e,.rtld_start
