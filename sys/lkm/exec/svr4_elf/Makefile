#	$NetBSD: Makefile,v 1.2 2001/07/18 16:53:34 mrg Exp $

.include "../Makefile.inc"

.if ${MACHINE} == sparc64 && ${MACHINE_ARCH} == sparc64
ELFSIZE=64
.else
ELFSIZE=32
.endif

.PATH:	$S/compat/svr4

CPPFLAGS+=	-nostdinc -I$S -D_KERNEL -DEXEC_ELF${ELFSIZE} -DELFSIZE=${ELFSIZE}

MKMAN=	no

KMOD=	exec_svr4_elf

SRCS=	lkminit_exec.c
SRCS+=	svr4_exec_elf${ELFSIZE}.c

.include <bsd.kmod.mk>
