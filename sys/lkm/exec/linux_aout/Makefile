#	$NetBSD: Makefile,v 1.1.4.1 2002/01/08 00:33:08 nathanw Exp $

.include "../Makefile.inc"

.PATH:	$S/compat/linux/common

CPPFLAGS+=	-nostdinc -I$S -D_KERNEL -DEXEC_AOUT -DCOMPAT_LINUX

KMOD=	exec_linux_aout

SRCS=	lkminit_exec.c
SRCS+=	linux_exec_aout.c

.include <bsd.kmod.mk>
