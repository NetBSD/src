#	$NetBSD: Makefile,v 1.5 2002/05/30 18:53:15 thorpej Exp $

.include "../Makefile.inc"

.PATH:	$S/kern

CPPFLAGS+=	-nostdinc -D_KERNEL -DEXEC_ELF32 -DELFSIZE=32

KMOD=	exec_elf

SRCS=	lkminit_exec.c
SRCS+=	exec_elf32.c core_elf32.c

.include <bsd.kmod.mk>
