#	$NetBSD: Makefile,v 1.1 2001/06/06 20:45:59 mrg Exp $

.include "../Makefile.inc"

.PATH:	$S/kern

CPPFLAGS+=	-nostdinc -I$S -D_KERNEL -DEXEC_ELF32 -DELFSIZE=32

MKMAN=	no

KMOD=	exec_elf

SRCS=	lkminit_exec.c
SRCS+=	exec_elf32.c

.include <bsd.kmod.mk>
