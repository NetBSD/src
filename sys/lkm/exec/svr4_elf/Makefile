#	$NetBSD: Makefile,v 1.1.2.2 2001/01/08 14:57:27 bouyer Exp $

.include "../Makefile.inc"

.PATH:	$S/compat/svr4

CPPFLAGS+=	-nostdinc -I$S -D_KERNEL -DEXEC_ELF32

MKMAN=	no

KMOD=	exec_svr4_elf

SRCS=	lkminit_exec.c
SRCS+=	svr4_exec_elf32.c

.include <bsd.kmod.mk>
