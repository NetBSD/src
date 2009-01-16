#	$NetBSD: Makefile.inc,v 1.11.2.1 2009/01/16 22:21:30 bouyer Exp $

SRCS+=		rtld_start.S mdreloc.c

# XXX Should not be in CPPFLAGS!
CPPFLAGS+=	-fpic

CPPFLAGS+=	-DELFSIZE=32

LDFLAGS+=	-Wl,-e,_rtld_start

.if defined(LD32DIR)
.include "Makefile.ld32"
.endif
