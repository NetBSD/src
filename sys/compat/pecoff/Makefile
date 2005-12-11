#	$NetBSD: Makefile,v 1.5 2005/12/11 12:20:23 christos Exp $

.include <bsd.sys.mk>		# for HOST_SH

DEP=	syscalls.conf syscalls.master ../../kern/makesyscalls.sh
OBJS=	pecoff_sysent.c pecoff_syscalls.c pecoff_syscall.h pecoff_syscallargs.h

${OBJS}: ${DEP}
	${HOST_SH} ../../kern/makesyscalls.sh syscalls.conf syscalls.master

all: ${OBJS}

.include <bsd.kinc.mk>
