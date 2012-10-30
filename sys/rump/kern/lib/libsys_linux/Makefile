#	$NetBSD: Makefile,v 1.1.2.2 2012/10/30 17:22:53 yamt Exp $
#

.PATH:	${.CURDIR}/../../../../compat/linux/common

LIB=	rumpkern_sys_linux

SRCS=	linux_blkio.c linux_cdrom.c linux_errno.c linux_fdio.c		\
	linux_file.c linux_hdio.c linux_ioctl.c linux_ipc.c linux_misc.c\
	linux_mtio.c linux_signal.c linux_signo.c linux_socket.c	\
	linux_sysctl.c linux_termios.c linux_time.c linux_file64.c

SRCS+=	rump_linux_sysent.c component.c

# XXX
CPPFLAGS+= -I${.CURDIR}

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
