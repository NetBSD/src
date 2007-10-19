#	$NetBSD: Makefile,v 1.10 2007/10/19 18:52:13 njoly Exp $

.include "../Makefile.inc"

.PATH:	$S/compat/linux/common
.PATH:	$S/compat/linux/arch/${MACHINE}
.PATH:	$S/compat/linux/arch/${MACHINE_ARCH}

CPPFLAGS+=	-nostdinc -D_KERNEL -DEXEC_ELF32 -DEXEC_ELF64

KMOD=	exec_linux_elf

SRCS=	lkminit_exec.c

.if ${MACHINE} == "amd64" || ${MACHINE} == "i386"
SRCS+=	linux_exec_machdep.c
.endif

.if ${MACHINE} == "alpha" || ${MACHINE} == "amd64" || ${MACHINE} == "sparc64"
CPPFLAGS+=	-DELFSIZE=64
SRCS+=	linux_exec_elf64.c
.else
CPPFLAGS+=	-DELFSIZE=32
SRCS+=	linux_exec_elf32.c
.endif

.include <bsd.kmod.mk>
