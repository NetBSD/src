#	$NetBSD: Makefile.inc,v 1.12.2.1 2013/02/25 00:28:03 tls Exp $

SRCS+=		rtld_start.S mdreloc.c

# XXX Should not be in CPPFLAGS!
CPPFLAGS+=	-fpic

CPPFLAGS+=	-DELFSIZE=32
.if ${MACHINE_ARCH} == "earm" || ${MACHINE_ARCH} == "earmeb" \
    || (!empty(CPUFLAGS) && ${CPUFLAGS:M-mabi=aapcs*} != "")
CPPFLAGS+=	-DHAVE_INITFINI_ARRAY
.endif

LDFLAGS+=	-Wl,-e,_rtld_start
