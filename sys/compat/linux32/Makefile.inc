#	$NetBSD: Makefile.inc,v 1.1.22.2 2006/09/09 02:45:52 rpaulo Exp $

.include <bsd.sys.mk>		# for HOST_SH

DEP=	syscalls.conf syscalls.master ../../../../kern/makesyscalls.sh
OBJS=	linux_sysent.c linux_syscalls.c linux_syscall.h linux_syscallargs.h

${OBJS}: ${DEP}
	${HOST_SH} ../../../../kern/makesyscalls.sh syscalls.conf syscalls.master

all: ${OBJS}
