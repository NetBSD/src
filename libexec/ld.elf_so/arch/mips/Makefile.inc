#	$NetBSD: Makefile.inc,v 1.21 2021/12/04 08:50:41 skrll Exp $

SRCS+=		rtld_start.S mips_reloc.c

COPTS+=		-G0

CPPFLAGS+=	-DELFSIZE=_MIPS_SZPTR
CPPFLAGS+=	-DRTLD_INHIBIT_COPY_RELOCS

AFLAGS+=	-Wa,--fatal-warnings

LDFLAGS+=	-Wl,-e,rtld_start
