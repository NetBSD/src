#	$NetBSD: Makefile,v 1.4.6.2 2004/09/18 14:54:09 skrll Exp $

.include "../Makefile.inc"

.PATH:	$S/compat/netbsd32

CPPFLAGS+=	-nostdinc -D_KERNEL -DEXEC_ELF32 -DCOMPAT_NETBSD32

KMOD=	exec_netbsd32_elf

SRCS=	lkminit_exec.c
SRCS+=	netbsd32_exec_elf32.c

.include <bsd.kmod.mk>
