#	$NetBSD: Makefile,v 1.4 1999/02/13 02:54:52 lukem Exp $

.include "../Makefile.inc"

.PATH:	$S/compat/sunos $S/arch/${MACHINE_ARCH}/${MACHINE_ARCH}

CPPFLAGS+= -DCOMPAT_SUNOS
MKMAN=	no
KMOD=	compat_sunos
SRCS=	sunos_exec.c sunos_ioctl.c sunos_misc.c sunos_syscalls.c \
	sunos_sysent.c sunos_machdep.c lkminit_emul.c

.include <bsd.kmod.mk>
