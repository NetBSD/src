#	$NetBSD: Makefile,v 1.2 2001/12/12 01:50:10 tv Exp $

.include "../Makefile.inc"

.PATH:	$S/kern

CPPFLAGS+=	-nostdinc -I$S -D_KERNEL -DEXEC_ELF32 -DELFSIZE=32

NOMAN=	# defined

KMOD=	exec_elf

SRCS=	lkminit_exec.c
SRCS+=	exec_elf32.c

.include <bsd.kmod.mk>
