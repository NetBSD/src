#	$NetBSD: Makefile,v 1.4.6.2 2004/11/29 07:24:51 skrll Exp $

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
