#	$NetBSD: Makefile,v 1.4.6.1 2004/11/14 08:16:12 skrll Exp $

.include "../Makefile.inc"

.PATH:	$S/compat/linux/common
.PATH:	$S/compat/linux/arch/${MACHINE_ARCH}

CPPFLAGS+=	-nostdinc -D_KERNEL -DEXEC_AOUT -DCOMPAT_LINUX

KMOD=	exec_linux_aout

SRCS=	lkminit_exec.c
SRCS+=	linux_exec_aout.c linux_exec_machdep.c

.include <bsd.kmod.mk>
