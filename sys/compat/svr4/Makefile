#	$NetBSD: Makefile,v 1.1.64.1 2004/08/03 10:44:32 skrll Exp $

.include <bsd.sys.mk>		# for HOST_SH

DEP=	syscalls.conf syscalls.master ../../kern/makesyscalls.sh
OBJS=	svr4_sysent.c svr4_syscalls.c svr4_syscall.h svr4_syscallargs.h

${OBJS}: ${DEP}
	${HOST_SH} ../../kern/makesyscalls.sh syscalls.conf syscalls.master
