#	$NetBSD: Makefile,v 1.4 2005/02/26 23:50:59 perry Exp $

.include <bsd.sys.mk>		# for HOST_SH

DEP=	syscalls.conf syscalls.master ../../kern/makesyscalls.sh
OBJS=	svr4_32_sysent.c svr4_32_syscalls.c svr4_32_syscall.h svr4_32_syscallargs.h

#syscalls.master: ../svr4/syscalls.master sed.script
#	sed -f sed.script < ../svr4/syscalls.master > syscalls.master

${OBJS}: ${DEP}
	${HOST_SH} ../../kern/makesyscalls.sh syscalls.conf syscalls.master

all: ${OBJS}

.include <bsd.kinc.mk>
