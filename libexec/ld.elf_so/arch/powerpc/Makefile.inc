#	$NetBSD: Makefile.inc,v 1.15.14.1 2018/04/07 04:12:09 pgoyette Exp $

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
LDFLAGS+=	-Wl,--script,${.CURDIR}/arch/powerpc/ld.so.script
.endif
