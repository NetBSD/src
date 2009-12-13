#	$NetBSD: Makefile.inc,v 1.13 2009/12/13 08:25:20 mrg Exp $

SRCS+=		rtld_start.S mdreloc.c

# XXX Should not be in CPPFLAGS!
CPPFLAGS+=	-fpic

CPPFLAGS+=	-DELFSIZE=32

LDFLAGS+=	-Wl,-e,_rtld_start

.if defined(MLIBDIR)
.include "Makefile.ld32"
.endif
