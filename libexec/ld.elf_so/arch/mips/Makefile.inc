#	$NetBSD: Makefile.inc,v 1.15.32.2 2009/09/05 03:49:02 matt Exp $

SRCS+=		rtld_start.S mips_reloc.c

# XXX Should not be in CPPFLAGS!
CPUFLAGS+=	-G0

.if (${CPUFLAGS:M-mabi=64} != "")
CPPFLAGS+=	-DELFSIZE=64
.else
CPPFLAGS+=	-DELFSIZE=32
.endif
CPPFLAGS+=	-DRTLD_INHIBIT_COPY_RELOCS
AFLAGS+=	-Wa,--fatal-warnings

LDFLAGS+=	-Wl,-e,.rtld_start
