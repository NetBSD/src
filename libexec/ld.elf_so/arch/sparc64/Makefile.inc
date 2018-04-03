#	$NetBSD: Makefile.inc,v 1.9 2018/04/03 21:10:27 joerg Exp $

SRCS+=		rtld_start.S mdreloc.c

# XXX Should not be in CPPFLAGS!
CPPFLAGS+=	-fpic

CPPFLAGS+=	-DELFSIZE=64
CPPFLAGS+=	-DRTLD_COMMON_CALL_IFUNC_RELA

LDFLAGS+=	-Wl,-e,_rtld_start
