#	$NetBSD: Makefile.inc,v 1.5 2002/09/09 19:28:36 mycroft Exp $

SRCS+=	rtld_start.S mdreloc.c

# XXX Should not be in CPPFLAGS!
CPPFLAGS+= -fpic

CPPFLAGS+= -DELFSIZE=32 -DVARPSZ

LDFLAGS+= -Bshareable -Bsymbolic -e _rtld_start
.if (${MACHINE_ARCH} == "arm")
LDFLAGS+= --script ${.CURDIR}/arch/arm/ld.so.script.le
.else
LDFLAGS+= --script ${.CURDIR}/arch/arm/ld.so.script.be
.endif
