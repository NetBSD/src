#	$NetBSD: Makefile,v 1.3.8.1 2002/06/20 16:02:29 gehenna Exp $

.include "../Makefile.inc"

.PATH:	$S/compat/ibcs2

CPPFLAGS+=	-nostdinc -D_KERNEL -DEXEC_ELF32

KMOD=	exec_ibcs2_elf

SRCS=	lkminit_exec.c
SRCS+=	ibcs2_exec_elf32.c

.include <bsd.kmod.mk>
