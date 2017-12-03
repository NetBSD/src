#	$NetBSD: Makefile,v 1.3.24.1 2017/12/03 11:38:51 jdolecek Exp $

.include "../Makefile.inc"
.include "../Makefile.assym"

KMOD=	compat_svr4

CPPFLAGS+=	-DSYSVSHM -DSYSVSEM -DSYSVMSG -DCOMPAT_SVR4

.PATH:	${S}/compat/svr4

SRCS+=	svr4_errno.c svr4_exec.c svr4_fcntl.c svr4_filio.c
SRCS+=	svr4_ioctl.c svr4_ipc.c svr4_lwp.c svr4_misc.c svr4_net.c
SRCS+=	svr4_resource.c svr4_schedctl.c svr4_signal.c svr4_signo.c
SRCS+=	svr4_socket.c svr4_sockio.c svr4_stat.c svr4_stream.c
SRCS+=	svr4_syscalls.c  svr4_sysent.c svr4_termios.c svr4_ttold.c
SRCS+=	svr4_mod.c

.include <bsd.kmodule.mk>
