#	$NetBSD: Makefile,v 1.3.8.1 2002/06/20 16:02:30 gehenna Exp $

.include "../Makefile.inc"

.PATH:	$S/compat/linux/common

CPPFLAGS+=	-nostdinc -D_KERNEL -DEXEC_AOUT -DCOMPAT_LINUX

KMOD=	exec_linux_aout

SRCS=	lkminit_exec.c
SRCS+=	linux_exec_aout.c

.include <bsd.kmod.mk>
