#	$NetBSD: Makefile,v 1.1.6.2 2002/06/23 17:50:00 jdolecek Exp $

.include "../Makefile.inc"

.PATH:	$S/compat/ibcs2

CPPFLAGS+=	-nostdinc -D_KERNEL -DEXEC_ELF32

KMOD=	exec_ibcs2_elf

SRCS=	lkminit_exec.c
SRCS+=	ibcs2_exec_elf32.c

.include <bsd.kmod.mk>
