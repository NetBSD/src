#	$NetBSD: Makefile,v 1.1 1996/08/28 06:17:25 mrg Exp $

.include "../Makefile.inc"

.PATH:	$S/compat/sunos $S/arch/${MACHINE}/${MACHINE}

CFLAGS+= -DCOMPAT_SUNOS
NOMAN=	yep
KMOD=	compat_sunos
SRCS=	sunos_exec.c sunos_ioctl.c sunos_misc.c sunos_syscalls.c \
	sunos_sysent.c sunos_machdep.c lkminit_emul.c

.include <bsd.kmod.mk>
