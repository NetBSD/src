# $NetBSD: Makefile.inc,v 1.2 2018/12/27 18:58:14 christos Exp $

SRCS+=		rtld_start.S mdreloc.c

# XXX Should not be in CPPFLAGS!
CPPFLAGS+=	-fpic

CPPFLAGS+=	-DELFSIZE=64
CPPFLAGS+=	-DELF_NOTE_MARCH_DESC=\"${LDELFSO_MACHINE_ARCH}\"

LDFLAGS+=	-Wl,-e,_rtld_start
