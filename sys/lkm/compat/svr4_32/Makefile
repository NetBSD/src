#	$NetBSD: Makefile,v 1.1.14.2 2002/04/01 18:48:09 nathanw Exp $

.include "../Makefile.inc"
USE_32=_32
.PATH:	$S/compat/svr4_32
.include "../svr4/Makefile.common"

# common source files
SRCS+=	lkminit_emul.c
SRCS+=	svr4_32_exec.c svr4_32_fcntl.c svr4_32_ioctl.c svr4_32_ipc.c \
	svr4_32_lwp.c svr4_32_misc.c svr4_32_net.c svr4_32_resource.c \
	svr4_32_signal.c svr4_32_socket.c svr4_32_sockio.c svr4_32_stat.c \
	svr4_32_stream.c svr4_32_syscalls.c svr4_32_sysent.c svr4_32_termios.c \
# ?
#	svr4_errno.c
#	svr4_filio.c
#	svr4_ttold.c

.if ${MACHINE} == "sparc64"
SRCS+=	svr4_32_sigcode.s svr4_32_machdep.c
CPPFLAGS+=	-D_LP64
AFLAGS+=	-D_LP64
.endif

.include <bsd.kmod.mk>
