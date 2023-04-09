#	$NetBSD: Makefile.inc,v 1.17 2023/04/09 17:24:48 chs Exp $

SRCS+=		ppc_reloc.c
LDFLAGS+=	-Wl,-e,_rtld_start

CPPFLAGS+=	-fPIC
CPPFLAGS+=	-DRTLD_COMMON_CALL_IFUNC_RELA

.if ${LDELFSO_MACHINE_ARCH} == "powerpc64"
SRCS+=		rtld_start64.S
CPPFLAGS+=	-DELFSIZE=64
.else
SRCS+=		rtld_start.S
CPPFLAGS+=	-DELFSIZE=32
.endif
