#	$NetBSD: Makefile,v 1.2 1996/10/22 19:09:50 explorer Exp $

.include "../Makefile.inc"

.PATH:	$S/compat/sunos $S/arch/${MACHINE_ARCH}/${MACHINE_ARCH}

CFLAGS+= -DCOMPAT_SUNOS
NOMAN=	yep
KMOD=	compat_sunos
SRCS=	sunos_exec.c sunos_ioctl.c sunos_misc.c sunos_syscalls.c \
	sunos_sysent.c sunos_machdep.c lkminit_emul.c

.include <bsd.kmod.mk>
