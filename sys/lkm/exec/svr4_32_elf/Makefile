#	$NetBSD: Makefile,v 1.4.6.1 2004/08/03 10:53:58 skrll Exp $

.include "../Makefile.inc"

.PATH:	$S/compat/svr4_32

CPPFLAGS+=	-nostdinc -D_KERNEL -DEXEC_ELF32 -DCOMPAT_NETBSD32

KMOD=	exec_svr4_32_elf

SRCS=	lkminit_exec.c
SRCS+=	svr4_32_exec_elf32.c

.include <bsd.kmod.mk>
