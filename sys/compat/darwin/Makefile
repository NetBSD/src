#	$NetBSD: Makefile,v 1.5 2005/12/11 12:19:56 christos Exp $

.include <bsd.sys.mk>		# for HOST_SH

DEP=	syscalls.conf syscalls.master ../../kern/makesyscalls.sh
OBJS=	darwin_sysent.c darwin_syscalls.c darwin_syscall.h darwin_syscallargs.h

${OBJS}: ${DEP}
	${HOST_SH} ../../kern/makesyscalls.sh syscalls.conf syscalls.master

all: ${OBJS}

.include <bsd.kinc.mk>
