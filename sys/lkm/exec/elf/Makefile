#	$NetBSD: Makefile,v 1.1.2.1 2002/01/10 20:00:57 thorpej Exp $

.include "../Makefile.inc"

.PATH:	$S/kern

CPPFLAGS+=	-nostdinc -I$S -D_KERNEL -DEXEC_ELF32 -DELFSIZE=32

KMOD=	exec_elf

SRCS=	lkminit_exec.c
SRCS+=	exec_elf32.c

.include <bsd.kmod.mk>
