#	$NetBSD: Makefile.inc,v 1.3.2.1 2004/05/28 08:31:23 tron Exp $

SRCS+=		rtld_start.S mdreloc.c

# XXX Should not be in CPPFLAGS!
CPPFLAGS+=	-fPIC

CPPFLAGS+=	-DELFSIZE=64

LDFLAGS+=	-Bshareable -Bsymbolic -e .rtld_start
