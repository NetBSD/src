#	$NetBSD: Makefile.inc,v 1.11 2008/10/26 07:11:54 mrg Exp $

SRCS+=		rtld_start.S mdreloc.c

# XXX Should not be in CPPFLAGS!
CPPFLAGS+=	-fpic

CPPFLAGS+=	-DELFSIZE=32
.if defined(LD32DIR)
CPPFLAGS+=	-DRTLD_ARCH_SUBDIR=\"sparc\"
.endif

LDFLAGS+=	-Wl,-e,_rtld_start
