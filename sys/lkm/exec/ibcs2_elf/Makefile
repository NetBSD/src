#	$NetBSD: Makefile,v 1.1.4.1 2002/01/08 00:33:07 nathanw Exp $

.include "../Makefile.inc"

.PATH:	$S/compat/ibcs2

CPPFLAGS+=	-nostdinc -I$S -D_KERNEL -DEXEC_ELF32

KMOD=	exec_ibcs2_elf

SRCS=	lkminit_exec.c
SRCS+=	ibcs2_exec_elf32.c

.include <bsd.kmod.mk>
