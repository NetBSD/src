#	$NetBSD: Makefile.inc,v 1.6 2002/09/10 14:19:18 mycroft Exp $

SRCS+=	rtld_start.S mdreloc.c

# XXX Should not be in CPPFLAGS!
CPPFLAGS+= -fpic

CPPFLAGS+= -DELFSIZE=32 -DVARPSZ

LDFLAGS+= -Bshareable -Bsymbolic -e _rtld_start
LDFLAGS+= --script ${.CURDIR}/arch/arm/ld.so.script
.if (${MACHINE_ARCH} == "armeb")
LDFLAGS+= -EB
.endif
