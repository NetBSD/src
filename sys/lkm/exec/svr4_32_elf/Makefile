#	$NetBSD: Makefile,v 1.3.8.2 2002/04/01 18:48:13 nathanw Exp $

.include "../Makefile.inc"

.PATH:	$S/compat/svr4_32

CPPFLAGS+=	-nostdinc -I$S -D_KERNEL -DEXEC_ELF32

KMOD=	exec_svr4_32_elf

SRCS=	lkminit_exec.c
SRCS+=	svr4_32_exec_elf32.c

.include <bsd.kmod.mk>
