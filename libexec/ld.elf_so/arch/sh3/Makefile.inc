#	$NetBSD: Makefile.inc,v 1.1.2.1 2004/05/28 08:31:22 tron Exp $

SRCS+=		rtld_start.S mdreloc.c

# XXX Should not be in CPPFLAGS!
CPPFLAGS+=	-fpic

CPPFLAGS+=	-DELFSIZE=32

LDFLAGS+=	-Bshareable -Bsymbolic -e .rtld_start
