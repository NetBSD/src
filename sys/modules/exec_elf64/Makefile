#	$NetBSD: Makefile,v 1.2.64.1 2019/06/10 22:09:24 christos Exp $

.include "../Makefile.inc"

KMOD=	exec_elf64

CPPFLAGS+=	-DEXEC_ELF64 -DCOREDUMP
.if ${MACHINE_ARCH} != "powerpc64" && \
    ${MACHINE_ARCH} != "sparc64"
CPPFLAGS+=	-DPAX_ASLR
.endif

.PATH:	${S}/kern
SRCS=	exec_elf64.c core_elf64.c

WARNS=	3

.include <bsd.kmodule.mk>
