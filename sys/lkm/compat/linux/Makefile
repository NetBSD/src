#	$NetBSD: Makefile,v 1.5 1997/10/26 21:58:34 lukem Exp $

.include "../Makefile.inc"

.PATH:	$S/compat/linux $S/arch/i386/i386

CPPFLAGS+=	-DVM86	-DNVT=1 -DEXEC_ELF32

NOMAN=	yep

KMOD=	compat_linux

SRCS=	linux_audio.c linux_misc.c linux_error.c linux_signal.c
SRCS+=	linux_exec.c linux_socket.c linux_file.c linux_syscalls.c
SRCS+=	linux_ioctl.c linux_sysent.c linux_ipc.c linux_termios.c

SRCS+=	linux_machdep.c

SRCS+=	lkminit_linux.c

.include <bsd.kmod.mk>
