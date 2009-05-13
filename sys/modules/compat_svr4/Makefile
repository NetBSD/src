#	$NetBSD: Makefile,v 1.2.6.1 2009/05/13 17:22:18 jym Exp $

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

.if ${MACHINE_ARCH} == "i386"
CPPFLAGS+=	-DEXEC_ELF32
SRCS+=	svr4_exec_elf32.c
.PATH:	${S}/arch/i386/i386
SRCS+=	svr4_machdep.c svr4_syscall.c svr4_sigcode.S
CPPFLAGS.svr4_sigcode.S+=	-D_LOCORE
.endif

.include <bsd.kmodule.mk>
