#	$NetBSD: Makefile,v 1.6.6.1 2015/04/06 15:18:29 skrll Exp $
#

.PATH:	${.CURDIR}/../../../../compat/linux/common

LIB=	rumpkern_sys_linux

SRCS=	linux_blkio.c linux_cdrom.c linux_errno.c linux_fdio.c		\
	linux_file.c linux_hdio.c linux_ioctl.c linux_ipc.c linux_misc.c\
	linux_mtio.c linux_signal.c linux_signo.c linux_socket.c	\
	linux_sysctl.c linux_termios.c linux_time.c linux_file64.c      \
	linux_pipe.c

SRCS+=	rump_linux_sysent.c linux_rump.c
SRCS+=	sys_linux_component.c

# XXX
CPPFLAGS+= -I${.CURDIR} -I${RUMPTOP}/librump/rumpkern
CPPFLAGS+= -DINET6

.include <bsd.lib.mk>
.include <bsd.klinks.mk>

SYS_P=rump_linux_sys
.include "../Makefile.sys"
