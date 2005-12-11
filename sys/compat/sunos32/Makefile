#	$NetBSD: Makefile,v 1.5 2005/12/11 12:20:23 christos Exp $

.include <bsd.sys.mk>		# for HOST_SH

DEP=	syscalls.conf syscalls.master ../../kern/makesyscalls.sh
OBJS=	sunos32_sysent.c sunos32_syscalls.c sunos32_syscall.h sunos32_syscallargs.h

${OBJS}: ${DEP}
	${HOST_SH} ../../kern/makesyscalls.sh syscalls.conf syscalls.master


all: ${OBJS}

.include <bsd.kinc.mk>
