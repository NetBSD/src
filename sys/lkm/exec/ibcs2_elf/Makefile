#	$NetBSD: Makefile,v 1.1 2000/12/08 23:05:38 jdolecek Exp $

.include "../Makefile.inc"

.PATH:	$S/compat/ibcs2

CPPFLAGS+=	-nostdinc -I$S -D_KERNEL -DEXEC_ELF32

MKMAN=	no

KMOD=	exec_ibcs2_elf

SRCS=	lkminit_exec.c
SRCS+=	ibcs2_exec_elf32.c

.include <bsd.kmod.mk>
