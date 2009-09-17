#	$NetBSD: Makefile,v 1.2 2009/09/17 23:07:12 jnemeth Exp $

.include "../Makefile.inc"

KMOD=	exec_elf32

CPPFLAGS+=	-DEXEC_ELF32 -DCOREDUMP
.if ${MACHINE_ARCH} != "arm" && \
    ${MACHINE_ARCH} != "armeb" && \
    ${MACHINE_ARCH} != "m68000" && \
    ${MACHINE_ARCH} != "m68k" && \
    ${MACHINE_ARCH} != "powerpc" && \
    ${MACHINE_ARCH} != "powerpc64" && \
    ${MACHINE_ARCH} != "sparc" && \
    ${MACHINE_ARCH} != "sparc64"
CPPFLAGS+=	-DPAX_ASLR
.endif

.PATH:	${S}/kern
SRCS=	exec_elf32.c core_elf32.c

.include <bsd.kmodule.mk>
