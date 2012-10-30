#	$NetBSD: Makefile.inc,v 1.11.42.1 2012/10/30 18:59:23 yamt Exp $

SRCS+=		rtld_start.S mdreloc.c

# XXX Should not be in CPPFLAGS!
CPPFLAGS+=	-fpic

CPPFLAGS+=	-DELFSIZE=32
.if ${MACHINE_ARCH} == "earm" || ${MACHINE_ARCH} == "earmeb"
CPPFLAGS+=	-DHAVE_INITFINI_ARRAY
.endif

LDFLAGS+=	-Wl,-e,_rtld_start
