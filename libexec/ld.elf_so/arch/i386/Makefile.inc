#	$NetBSD: Makefile.inc,v 1.15 2018/04/03 21:10:27 joerg Exp $

SRCS+=		rtld_start.S mdreloc.c

# XXX Should not be in CPPFLAGS!
CPPFLAGS+=	-fpic

CPPFLAGS+=	-DELFSIZE=32
CPPFLAGS+=	-DRTLD_COMMON_CALL_IFUNC_REL

LDFLAGS+=	-Wl,-e,.rtld_start
