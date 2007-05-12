#	$NetBSD: Makefile,v 1.9 2007/05/12 10:45:01 dsl Exp $

.include "../Makefile.inc"

.PATH:	$S/compat/linux/common
.PATH:	$S/compat/linux/arch/${MACHINE_ARCH}

CPPFLAGS+=	-nostdinc -D_KERNEL -DEXEC_ELF32 -DEXEC_ELF64

KMOD=	exec_linux_elf

SRCS=	lkminit_exec.c

.if ${MACHINE} == "i386"
SRCS+=	linux_exec_machdep.c
.endif

.if ${MACHINE} == "alpha" || ${MACHINE} == "sparc64"
SRCS+=	linux_exec_elf64.c
.else
SRCS+=	linux_exec_elf32.c
.endif

.include <bsd.kmod.mk>
