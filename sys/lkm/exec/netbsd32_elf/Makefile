#	$NetBSD: Makefile,v 1.6 2005/12/11 12:24:48 christos Exp $

.include "../Makefile.inc"

.PATH:	$S/compat/netbsd32

CPPFLAGS+=	-nostdinc -D_KERNEL -DEXEC_ELF32 -DCOMPAT_NETBSD32

KMOD=	exec_netbsd32_elf

SRCS=	lkminit_exec.c
SRCS+=	netbsd32_exec_elf32.c

.include <bsd.kmod.mk>
