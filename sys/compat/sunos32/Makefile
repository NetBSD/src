#	$NetBSD: Makefile,v 1.4 2005/02/26 23:49:20 perry Exp $

.include <bsd.sys.mk>		# for HOST_SH

DEP=	syscalls.conf syscalls.master ../../kern/makesyscalls.sh
OBJS=	sunos32_sysent.c sunos32_syscalls.c sunos32_syscall.h sunos32_syscallargs.h

${OBJS}: ${DEP}
	${HOST_SH} ../../kern/makesyscalls.sh syscalls.conf syscalls.master


all: ${OBJS}

.include <bsd.kinc.mk>
