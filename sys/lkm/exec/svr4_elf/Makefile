#	$NetBSD: Makefile,v 1.1 2000/12/08 23:05:47 jdolecek Exp $

.include "../Makefile.inc"

.PATH:	$S/compat/svr4

CPPFLAGS+=	-nostdinc -I$S -D_KERNEL -DEXEC_ELF32

MKMAN=	no

KMOD=	exec_svr4_elf

SRCS=	lkminit_exec.c
SRCS+=	svr4_exec_elf32.c

.include <bsd.kmod.mk>
