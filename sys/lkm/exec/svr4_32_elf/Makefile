#	$NetBSD: Makefile,v 1.2 2001/12/12 01:50:13 tv Exp $

.include "../Makefile.inc"

.PATH:	$S/compat/svr4_32

CPPFLAGS+=	-nostdinc -I$S -D_KERNEL -DEXEC_ELF32

NOMAN=	# defined

KMOD=	exec_svr4_32_elf

SRCS=	lkminit_exec.c
SRCS+=	svr4_32_exec_elf32.c

.include <bsd.kmod.mk>
