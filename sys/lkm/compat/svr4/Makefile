#	$NetBSD: Makefile,v 1.3 1999/02/13 02:54:52 lukem Exp $

.include "../Makefile.inc"

.PATH:	$S/compat/svr4 $S/arch/${MACHINE}/${MACHINE}

CPPFLAGS+= -DCOMPAT_SVR4
MKMAN=	no
KMOD=	compat_svr4
SRCS=	svr4_error.c svr4_exec.c svr4_fcntl.c svr4_filio.c svr4_ioctl.c \
	svr4_ipc.c svr4_misc.c svr4_net.c svr4_signal.c svr4_socket.c \
	svr4_sockio.c svr4_stat.c svr4_stream.c svr4_syscalls.c svr4_sysent.c \
	svr4_termios.c svr4_ttold.c svr4_machdep.o

.include <bsd.kmod.mk>
