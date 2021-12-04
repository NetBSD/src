# $NetBSD: Makefile.inc,v 1.3 2021/12/04 08:53:34 skrll Exp $

SRCS+=		rtld_start.S mdreloc.c

# XXX Should not be in CPPFLAGS!
CPPFLAGS+=	-fpic

CPPFLAGS+=	-DELFSIZE=64
CPPFLAGS+=	-DELF_NOTE_MARCH_DESC=\"${LDELFSO_MACHINE_ARCH}\"
CPPFLAGS+=	-DRTLD_ARCH_CALL_IFUNC

LDFLAGS+=	-Wl,-e,_rtld_start
