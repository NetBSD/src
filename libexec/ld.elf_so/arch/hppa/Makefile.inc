#	$NetBSD: Makefile.inc,v 1.6 2004/05/17 13:16:02 skrll Exp $

SRCS+=		rtld_start.S hppa_reloc.c

# XXX Should not be in CPPFLAGS!
CPPFLAGS+=	-fpic

CPPFLAGS+=	-DELFSIZE=32
#CPPFLAGS+=	-DDEBUG
#CPPFLAGS+=	-DRTLD_DEBUG
#CPPFLAGS+=	-DRTLD_DEBUG_RELOC
#CPPFLAGS+=	-DRTLD_DEBUG_HPPA

LDFLAGS+=	-e '$$$$rtld_start'
