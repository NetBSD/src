#	$NetBSD: Makefile.inc,v 1.5 2004/02/23 21:20:40 skrll Exp $

SRCS+=		rtld_start.S hppa_reloc.c

# XXX Should not be in CPPFLAGS!
CPPFLAGS+=	-fpic

CPPFLAGS+=	-DELFSIZE=32
#CPPFLAGS+=	-DRTLD_DEBUG
#CPPFLAGS+=	-DRTLD_DEBUG_RELOC
#CPPFLAGS+=	-DRTLD_DEBUG_HPPA

LDFLAGS+=	-e '$$$$rtld_start'
LDFLAGS+=	--script ${.CURDIR}/arch/hppa/ld.so.script
