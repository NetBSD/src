#	$NetBSD: Makefile,v 1.4.8.2 2002/04/01 18:48:11 nathanw Exp $

.include "../Makefile.inc"

.PATH:	$S/kern

CPPFLAGS+=	-nostdinc -I$S -D_KERNEL -DEXEC_ELF32 -DELFSIZE=32

KMOD=	exec_elf

SRCS=	lkminit_exec.c
SRCS+=	exec_elf32.c core_elf32.c

.include <bsd.kmod.mk>
