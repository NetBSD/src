#	$NetBSD: Makefile,v 1.1.4.1 2002/01/08 00:33:12 nathanw Exp $

.include "../Makefile.inc"

.PATH:	$S/compat/netbsd32

CPPFLAGS+=	-nostdinc -I$S -D_KERNEL -DEXEC_ELF32

KMOD=	exec_netbsd32_elf

SRCS=	lkminit_exec.c
SRCS+=	netbsd32_exec_elf32.c

.include <bsd.kmod.mk>
