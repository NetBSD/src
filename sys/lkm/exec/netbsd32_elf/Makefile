#	$NetBSD: Makefile,v 1.3 2001/12/12 11:49:07 lukem Exp $

.include "../Makefile.inc"

.PATH:	$S/compat/netbsd32

CPPFLAGS+=	-nostdinc -I$S -D_KERNEL -DEXEC_ELF32

KMOD=	exec_netbsd32_elf

SRCS=	lkminit_exec.c
SRCS+=	netbsd32_exec_elf32.c

.include <bsd.kmod.mk>
