#	$NetBSD: Makefile,v 1.5 1997/10/26 21:58:33 lukem Exp $

.include "../Makefile.inc"

.PATH:	$S/compat/freebsd $S/arch/i386/i386

CPPFLAGS+=	-DVM86

NOMAN=	yep

KMOD=	compat_freebsd

SRCS=	freebsd_exec.c freebsd_misc.c freebsd_sysent.c freebsd_file.c
SRCS+=	freebsd_ptrace.c freebsd_ioctl.c freebsd_syscalls.c

SRCS+=	freebsd_machdep.c

SRCS+=	lkminit_emul.c

.include <bsd.kmod.mk>
