#	$NetBSD: Makefile,v 1.1 1996/08/22 20:18:15 explorer Exp $

.PATH:	/sys/compat/freebsd /sys/arch/i386/i386

CFLAGS+=	-DVM86

NOMAN=	yep

KMOD=	compat_freebsd

SRCS=	freebsd_exec.c freebsd_misc.c freebsd_sysent.c freebsd_file.c
SRCS+=	freebsd_ptrace.c freebsd_ioctl.c freebsd_syscalls.c

SRCS+=	freebsd_machdep.c

SRCS+=	lkminit_emul.c

.include <bsd.kmod.mk>
