# $NetBSD: Makefile.inc,v 1.1.14.1 2019/01/18 08:50:11 pgoyette Exp $

SRCS+=		rtld_start.S mdreloc.c

# XXX Should not be in CPPFLAGS!
CPPFLAGS+=	-fpic

.if ${LDELFSO_MACHINE_ARCH} == "riscv64"
CPPFLAGS+=	-DELFSIZE=64
.else
CPPFLAGS+=	-DELFSIZE=32
.endif

LDFLAGS+=	-Wl,-e,_rtld_start
