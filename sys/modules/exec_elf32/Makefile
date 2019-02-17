#	$NetBSD: Makefile,v 1.4 2019/02/17 13:21:55 rin Exp $

.include "../Makefile.inc"

KMOD=	exec_elf32

CPPFLAGS+=	-DEXEC_ELF32 -DCOREDUMP
.if ${MACHINE_ARCH} != "arm" && \
    ${MACHINE_ARCH} != "armeb" && \
    ${MACHINE_ARCH} != "m68000" && \
    ${MACHINE_ARCH} != "m68k" && \
    ${MACHINE_ARCH} != "powerpc" && \
    ${MACHINE_ARCH} != "powerpc64" && \
    ${MACHINE_ARCH} != "sh3eb" && \
    ${MACHINE_ARCH} != "sh3el" && \
    ${MACHINE_ARCH} != "sparc" && \
    ${MACHINE_ARCH} != "sparc64"
CPPFLAGS+=	-DPAX_ASLR
.endif

.PATH:	${S}/kern
SRCS=	exec_elf32.c core_elf32.c

WARNS=	3

.include <bsd.kmodule.mk>
