#	$NetBSD: Makefile,v 1.1 2001/06/05 14:49:00 mrg Exp $

.include "../Makefile.inc"

.PATH:	$S/compat/svr4_32

CPPFLAGS+=	-nostdinc -I$S -D_KERNEL -DEXEC_ELF32

MKMAN=	no

KMOD=	exec_svr4_32_elf

SRCS=	lkminit_exec.c
SRCS+=	svr4_32_exec_elf32.c

.include <bsd.kmod.mk>
