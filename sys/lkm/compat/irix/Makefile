#	$NetBSD: Makefile,v 1.1.2.2 2002/12/11 06:46:26 thorpej Exp $

.include "../Makefile.inc"

.PATH:	$S/compat/irix $S/arch/${MACHINE}/${MACHINE} $S/arch/${MACHINE_CPU}/${MACHINE_CPU} $S/compat/common

CPPFLAGS+= -DCOMPAT_IRIX -DSYSVSEM -DSYSVMSG -DSYSVSHM -nostdinc
AFLAGS=		-x assembler-with-cpp -traditional-cpp -D_LOCORE \
		-I. -I${S}

KMOD=	compat_irix

# common source files
SRCS+=	lkminit_emul.c
SRCS+=	irix_dirent.c irix_errno.c irix_exec.c		\
	irix_fcntl.c irix_ioctl.c irix_kmem.c irix_misc.c irix_mman.c	\
	irix_mount.c irix_prctl.c irix_resource.c irix_signal.c		\
	irix_stat.c irix_swap.c	irix_syscalls.c irix_sysctl.c		\
	irix_sysent.c irix_sysmp.c irix_syssgi.c irix_systeminfo.c	\
	irix_usema.c

# items pulled from compat/svr4
.PATH: $S/compat/svr4
SRCS+=	svr4_fcntl.c svr4_filio.c svr4_ioctl.c svr4_ipc.c svr4_misc.c	\
	svr4_net.c svr4_signal.c svr4_signo.c svr4_socket.c		\
	svr4_sockio.c svr4_stat.c svr4_stream.c svr4_termios.c svr4_ttold.c

# MD files
SRCS+=	irix_syscall.c
SRCS+=	svr4_machdep.c

.include <bsd.kmod.mk>
