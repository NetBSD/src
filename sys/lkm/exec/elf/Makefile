#	$NetBSD: Makefile,v 1.4 2002/02/10 16:25:49 mrg Exp $

.include "../Makefile.inc"

.PATH:	$S/kern

CPPFLAGS+=	-nostdinc -I$S -D_KERNEL -DEXEC_ELF32 -DELFSIZE=32

KMOD=	exec_elf

SRCS=	lkminit_exec.c
SRCS+=	exec_elf32.c core_elf32.c

.include <bsd.kmod.mk>
