#	$NetBSD: Makefile.inc,v 1.12 2009/01/06 02:15:06 mrg Exp $

SRCS+=		rtld_start.S mdreloc.c

# XXX Should not be in CPPFLAGS!
CPPFLAGS+=	-fpic

CPPFLAGS+=	-DELFSIZE=32

LDFLAGS+=	-Wl,-e,.rtld_start

.if defined(LD32DIR)
.include "Makefile.ld32"
.endif
