# $NetBSD: Makefile.inc,v 1.2 2018/12/27 18:58:14 christos Exp $

SRCS+=		rtld_start.S mdreloc.c

# XXX Should not be in CPPFLAGS!
CPPFLAGS+=	-fpic

.if ${LDELFSO_MACHINE_ARCH} == "riscv64"
CPPFLAGS+=	-DELFSIZE=64
.else
CPPFLAGS+=	-DELFSIZE=32
.endif

LDFLAGS+=	-Wl,-e,_rtld_start
