#	$NetBSD: Makefile,v 1.4.10.1 2002/06/20 16:02:27 gehenna Exp $

.include "../Makefile.inc"

.PATH:	$S/kern

CPPFLAGS+=	-nostdinc -D_KERNEL -DEXEC_ELF32 -DELFSIZE=32

KMOD=	exec_elf

SRCS=	lkminit_exec.c
SRCS+=	exec_elf32.c core_elf32.c

.include <bsd.kmod.mk>
