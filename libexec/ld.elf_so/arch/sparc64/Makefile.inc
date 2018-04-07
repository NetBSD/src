#	$NetBSD: Makefile.inc,v 1.8.82.1 2018/04/07 04:12:09 pgoyette Exp $

SRCS+=		rtld_start.S mdreloc.c

# XXX Should not be in CPPFLAGS!
CPPFLAGS+=	-fpic

CPPFLAGS+=	-DELFSIZE=64
CPPFLAGS+=	-DRTLD_COMMON_CALL_IFUNC_RELA

LDFLAGS+=	-Wl,-e,_rtld_start
