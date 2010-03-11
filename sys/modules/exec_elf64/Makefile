#	$NetBSD: Makefile,v 1.1.10.3 2010/03/11 15:04:25 yamt Exp $

.include "../Makefile.inc"

KMOD=	exec_elf64

CPPFLAGS+=	-DEXEC_ELF64 -DCOREDUMP
.if ${MACHINE_ARCH} != "powerpc64" && \
    ${MACHINE_ARCH} != "sparc64"
CPPFLAGS+=	-DPAX_ASLR
.endif

.PATH:	${S}/kern
SRCS=	exec_elf64.c core_elf64.c

.include <bsd.kmodule.mk>
