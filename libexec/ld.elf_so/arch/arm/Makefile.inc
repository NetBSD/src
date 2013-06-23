#	$NetBSD: Makefile.inc,v 1.12.2.2 2013/06/23 06:28:50 tls Exp $

SRCS+=		rtld_start.S mdreloc.c

# XXX Should not be in CPPFLAGS!
CPPFLAGS+=	-fpic

CPPFLAGS+=	-DELFSIZE=32
.if !empty(LDELFSO_MACHINE_ARCH:Mearm*)
CPPFLAGS+=	-DHAVE_INITFINI_ARRAY
SRCS+=		find_exidx.c
.endif

LDFLAGS+=	-Wl,-e,_rtld_start
