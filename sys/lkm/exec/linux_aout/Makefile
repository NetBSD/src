#	$NetBSD: Makefile,v 1.5 2004/11/13 06:09:47 christos Exp $

.include "../Makefile.inc"

.PATH:	$S/compat/linux/common
.PATH:	$S/compat/linux/arch/${MACHINE_ARCH}

CPPFLAGS+=	-nostdinc -D_KERNEL -DEXEC_AOUT -DCOMPAT_LINUX

KMOD=	exec_linux_aout linux_exec_machdep.c

SRCS=	lkminit_exec.c
SRCS+=	linux_exec_aout.c

.include <bsd.kmod.mk>
