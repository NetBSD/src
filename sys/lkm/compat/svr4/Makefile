#	$NetBSD: Makefile,v 1.5 2000/12/09 11:21:41 jdolecek Exp $

.include "../Makefile.inc"

.PATH:	$S/compat/svr4 $S/arch/${MACHINE_ARCH}/${MACHINE_ARCH}

CPPFLAGS+= -DCOMPAT_SVR4 -DSYSVSEM -DSYSVMSG -DSYSVSHM -DSVR4_COMPAT_SOLARIS2
AFLAGS=         -x assembler-with-cpp -traditional-cpp -D_LOCORE \
                -I. -I${S}

MKMAN=	no
KMOD=	compat_svr4

# common source files
SRCS+=	lkminit_emul.c
SRCS+=	svr4_errno.c svr4_exec.c svr4_fcntl.c svr4_filio.c svr4_ioctl.c \
	svr4_ipc.c svr4_lwp.c svr4_misc.c svr4_net.c svr4_resource.c	\
	svr4_signal.c svr4_socket.c svr4_sockio.c svr4_stat.c svr4_stream.c \
	svr4_syscalls.c svr4_sysent.c svr4_termios.c svr4_ttold.c
SRCS+=	svr4_machdep.c

.if ${MACHINE} == "i386"
SRCS+=	svr4_sigcode.s svr4_syscall.c
.endif

.include <bsd.kmod.mk>
