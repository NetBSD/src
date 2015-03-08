#	$NetBSD: Makefile,v 1.7 2015/03/08 15:07:33 christos Exp $
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
SYS_MKSYSCALLS=${.CURDIR}/../../../../kern/makesyscalls.sh
SYS_DST=${SYS_P}calls.c ${SYS_P}ent.c ${SYS_P}callargs.h ${SYS_P}call.h: 
SYS_SRC=${SYS_MKSYSCALLS} syscalls.conf syscalls.master

${SYS_DST}: ${SYS_SRC}
	${HOST_SH} ${.ALLSRC}
