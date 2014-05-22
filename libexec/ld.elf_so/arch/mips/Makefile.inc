#	$NetBSD: Makefile.inc,v 1.19.6.1 2014/05/22 11:37:14 yamt Exp $

SRCS+=		rtld_start.S mips_reloc.c

COPTS+=		-G0

CPPFLAGS+=	-DELFSIZE=_MIPS_SZPTR
CPPFLAGS+=	-DRTLD_INHIBIT_COPY_RELOCS
AFLAGS+=	-Wa,--fatal-warnings

LDFLAGS+=	-Wl,-e,rtld_start
