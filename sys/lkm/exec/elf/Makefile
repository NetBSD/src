#	$NetBSD: Makefile,v 1.4.8.3 2002/06/20 03:47:43 nathanw Exp $

.include "../Makefile.inc"

.PATH:	$S/kern

CPPFLAGS+=	-nostdinc -D_KERNEL -DEXEC_ELF32 -DELFSIZE=32

KMOD=	exec_elf

SRCS=	lkminit_exec.c
SRCS+=	exec_elf32.c core_elf32.c

.include <bsd.kmod.mk>
