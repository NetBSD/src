#	$NetBSD: Makefile.inc,v 1.17.4.1 2026/05/06 17:18:04 martin Exp $

SRCS+=		ppc_reloc.c fixup.c
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
