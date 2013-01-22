#	$NetBSD: Makefile.inc,v 1.13 2013/01/22 22:49:09 matt Exp $

SRCS+=		rtld_start.S mdreloc.c

# XXX Should not be in CPPFLAGS!
CPPFLAGS+=	-fpic

CPPFLAGS+=	-DELFSIZE=32
.if ${MACHINE_ARCH} == "earm" || ${MACHINE_ARCH} == "earmeb" || \
  ${CFLAGS:Mabi=aapcs} != ""
CPPFLAGS+=	-DHAVE_INITFINI_ARRAY
.endif

LDFLAGS+=	-Wl,-e,_rtld_start
