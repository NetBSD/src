#	$NetBSD: Makefile,v 1.5 2003/10/16 16:06:43 christos Exp $

.include "../Makefile.inc"

.PATH:	$S/compat/svr4_32

CPPFLAGS+=	-nostdinc -D_KERNEL -DEXEC_ELF32 -DCOMPAT_NETBSD32

KMOD=	exec_svr4_32_elf

SRCS=	lkminit_exec.c
SRCS+=	svr4_32_exec_elf32.c

.include <bsd.kmod.mk>
