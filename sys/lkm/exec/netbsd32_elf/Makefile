#	$NetBSD: Makefile,v 1.4 2002/05/30 18:53:20 thorpej Exp $

.include "../Makefile.inc"

.PATH:	$S/compat/netbsd32

CPPFLAGS+=	-nostdinc -D_KERNEL -DEXEC_ELF32

KMOD=	exec_netbsd32_elf

SRCS=	lkminit_exec.c
SRCS+=	netbsd32_exec_elf32.c

.include <bsd.kmod.mk>
