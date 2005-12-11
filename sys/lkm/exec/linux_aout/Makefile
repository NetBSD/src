#	$NetBSD: Makefile,v 1.8 2005/12/11 12:24:48 christos Exp $

.include "../Makefile.inc"

.PATH:	$S/compat/linux/common
.PATH:	$S/compat/linux/arch/${MACHINE_ARCH}

CPPFLAGS+=	-nostdinc -D_KERNEL -DEXEC_AOUT -DCOMPAT_LINUX

KMOD=	exec_linux_aout

SRCS=	lkminit_exec.c
SRCS+=	linux_exec_aout.c

.if ${MACHINE} == "i386"
SRCS+=	linux_exec_machdep.c
.endif

.include <bsd.kmod.mk>
