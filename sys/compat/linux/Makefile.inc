#	$NetBSD: Makefile.inc,v 1.2.46.1 2004/08/03 10:43:53 skrll Exp $

.include <bsd.sys.mk>		# for HOST_SH

.include <bsd.sys.mk>		# for HOST_SH

DEP=	syscalls.conf syscalls.master ../../../../kern/makesyscalls.sh
OBJS=	linux_sysent.c linux_syscalls.c linux_syscall.h linux_syscallargs.h

${OBJS}: ${DEP}
	${HOST_SH} ../../../../kern/makesyscalls.sh syscalls.conf syscalls.master

all: ${OBJS}
