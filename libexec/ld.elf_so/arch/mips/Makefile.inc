#	$NetBSD: Makefile.inc,v 1.17 2009/12/14 00:41:18 matt Exp $

SRCS+=		rtld_start.S mips_reloc.c

# XXX Should not be in CPPFLAGS!
CPUFLAGS+=	-G0

ABI64?= ${CPUFLAGS:M-mabi=64:M-mabi=o64}
.if !empty(ABI64)
CPPFLAGS+=	-DELFSIZE=64
.else
CPPFLAGS+=	-DELFSIZE=32
.endif
CPPFLAGS+=	-DRTLD_INHIBIT_COPY_RELOCS
AFLAGS+=	-Wa,--fatal-warnings

LDFLAGS+=	-Wl,-e,rtld_start
