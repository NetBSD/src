#	$NetBSD: Makefile,v 1.2.6.4 2017/12/03 11:39:15 jdolecek Exp $
#

.PATH:	${.CURDIR}/../../../../compat/linux/common

LIB=	rumpkern_sys_linux
COMMENT=Linux system call translation

SRCS=	linux_blkio.c linux_cdrom.c linux_errno.c linux_fdio.c		\
	linux_file.c linux_hdio.c linux_ioctl.c linux_ipc.c linux_misc.c\
	linux_mtio.c linux_signal.c linux_signo.c linux_socket.c	\
	linux_sysctl.c linux_termios.c linux_time.c linux_file64.c      \
	linux_pipe.c

SRCS+=	linux_rump.c
SRCS+=	sys_linux_component.c

# XXX
CPPFLAGS+= -DINET6

SYS_P=rump_linux_sys
.include "../Makefile.sys"

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
