#	$NetBSD: Makefile.inc,v 1.5 2002/09/09 18:10:20 mycroft Exp $

SRCS+=	rtld_start.S mdreloc.c

# XXX Should not be in CPPFLAGS!
CPPFLAGS+= -fpic

CPPFLAGS+= -DELFSIZE=32 -DVARPSZ

LDFLAGS+= -Bshareable -Bsymbolic -e _rtld_start
LDFLAGS+= --script ${.CURDIR}/arch/sparc/ld.so.script
