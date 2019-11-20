#	$NetBSD: Makefile,v 1.4 2019/11/20 19:37:54 pgoyette Exp $

.include "../Makefile.inc"

KMOD=	exec_elf64

CPPFLAGS+=	-DEXEC_ELF64
.if ${MACHINE_ARCH} != "powerpc64" && \
    ${MACHINE_ARCH} != "sparc64"
CPPFLAGS+=	-DPAX_ASLR
.endif

.PATH:	${S}/kern
SRCS=	exec_elf64.c core_elf64.c

WARNS=	3

.include <bsd.kmodule.mk>
