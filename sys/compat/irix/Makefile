#	$NetBSD: Makefile,v 1.4 2005/02/26 23:38:14 perry Exp $

.include <bsd.sys.mk>		# for HOST_SH

DEP=	syscalls.conf syscalls.master ../../kern/makesyscalls.sh
OBJS=	irix_sysent.c irix_syscalls.c irix_syscall.h irix_syscallargs.h

${OBJS}: ${DEP}
	${HOST_SH} ../../kern/makesyscalls.sh syscalls.conf syscalls.master

all: ${OBJS}

.include <bsd.kinc.mk>
