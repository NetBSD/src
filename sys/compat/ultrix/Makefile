#	$NetBSD: Makefile,v 1.1.64.1 2004/08/03 10:44:35 skrll Exp $

.include <bsd.sys.mk>		# for HOST_SH

DEP=	syscalls.conf syscalls.master ../../kern/makesyscalls.sh
OBJS=	ultrix_sysent.c ultrix_syscalls.c ultrix_syscall.h ultrix_syscallargs.h

${OBJS}: ${DEP}
	${HOST_SH} ../../kern/makesyscalls.sh syscalls.conf syscalls.master
