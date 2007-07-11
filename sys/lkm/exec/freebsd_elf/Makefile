#	$NetBSD: Makefile,v 1.6.66.1 2007/07/11 20:10:36 mjf Exp $

.include "../Makefile.inc"

.PATH:	$S/compat/freebsd

CPPFLAGS+=	-nostdinc -D_KERNEL -DEXEC_ELF32

KMOD=	exec_freebsd_elf

SRCS=	lkminit_exec.c
SRCS+=	freebsd_exec_elf32.c

.include <bsd.kmod.mk>
