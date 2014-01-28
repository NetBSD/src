#	$NetBSD: Makefile,v 1.5 2014/01/28 10:54:45 njoly Exp $
#

.PATH:	${.CURDIR}/../../../../compat/linux/common

LIB=	rumpkern_sys_linux

SRCS=	linux_blkio.c linux_cdrom.c linux_errno.c linux_fdio.c		\
	linux_file.c linux_hdio.c linux_ioctl.c linux_ipc.c linux_misc.c\
	linux_mtio.c linux_signal.c linux_signo.c linux_socket.c	\
	linux_sysctl.c linux_termios.c linux_time.c linux_file64.c      \
	linux_pipe.c

SRCS+=	rump_linux_sysent.c component.c linux_rump.c

# XXX
CPPFLAGS+= -I${.CURDIR} -I${RUMPTOP}/librump/rumpkern
CPPFLAGS+= -DINET6

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
