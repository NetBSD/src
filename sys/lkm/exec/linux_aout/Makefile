#	$NetBSD: Makefile,v 1.1.4.2 2002/06/20 03:47:46 nathanw Exp $

.include "../Makefile.inc"

.PATH:	$S/compat/linux/common

CPPFLAGS+=	-nostdinc -D_KERNEL -DEXEC_AOUT -DCOMPAT_LINUX

KMOD=	exec_linux_aout

SRCS=	lkminit_exec.c
SRCS+=	linux_exec_aout.c

.include <bsd.kmod.mk>
