#	$NetBSD: Makefile.inc,v 1.7 2005/06/04 16:17:17 lukem Exp $

SRCS+=		rtld_start.S hppa_reloc.c

# XXX Should not be in CPPFLAGS!
CPPFLAGS+=	-fpic

CPPFLAGS+=	-DELFSIZE=32
#CPPFLAGS+=	-DDEBUG
#CPPFLAGS+=	-DRTLD_DEBUG
#CPPFLAGS+=	-DRTLD_DEBUG_RELOC
#CPPFLAGS+=	-DRTLD_DEBUG_HPPA

LDFLAGS+=	-Wl,-e,'$$$$rtld_start'
