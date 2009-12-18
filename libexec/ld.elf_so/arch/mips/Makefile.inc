#	$NetBSD: Makefile.inc,v 1.18 2009/12/18 17:10:07 mrg Exp $

SRCS+=		rtld_start.S mips_reloc.c

COPTS+=		-G0

ABI64?= ${CFLAGS:M-mabi=64}
ABIO64?= ${CFLAGS:M-mabi=o64}
.if !empty(ABI64) || !empty(ABIO64)
CPPFLAGS+=	-DELFSIZE=64
.else
CPPFLAGS+=	-DELFSIZE=32
.endif
CPPFLAGS+=	-DRTLD_INHIBIT_COPY_RELOCS
AFLAGS+=	-Wa,--fatal-warnings

LDFLAGS+=	-Wl,-e,rtld_start
