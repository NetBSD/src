#	$NetBSD: Makefile.inc,v 1.13 2014/03/06 07:47:39 matt Exp $

SRCS+=		ppc_reloc.c
LDFLAGS+=	-Wl,-e,_rtld_start

# XXX Should not be in CPPFLAGS!
.if ${LDELFSO_MACHINE_ARCH} == "powerpc64"
SRCS+=		rtld_start64.S
CPPFLAGS+=	-DELFSIZE=64
.else
SRCS+=		rtld_start.S
CPPFLAGS+=	-fpic
CPPFLAGS+=	-DELFSIZE=32
LDFLAGS+=	-Wl,--script,${.CURDIR}/arch/powerpc/ld.so.script
.endif

