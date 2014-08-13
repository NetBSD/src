#	$NetBSD: Makefile.inc,v 1.14 2014/08/13 15:56:52 joerg Exp $

SRCS+=		ppc_reloc.c
LDFLAGS+=	-Wl,-e,_rtld_start

# XXX Should not be in CPPFLAGS!
CPPFLAGS+=	-fpic

.if ${LDELFSO_MACHINE_ARCH} == "powerpc64"
SRCS+=		rtld_start64.S
CPPFLAGS+=	-DELFSIZE=64
.else
SRCS+=		rtld_start.S
CPPFLAGS+=	-DELFSIZE=32
LDFLAGS+=	-Wl,--script,${.CURDIR}/arch/powerpc/ld.so.script
.endif
