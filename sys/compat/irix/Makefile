#	$NetBSD: Makefile,v 1.1.24.1 2004/08/03 10:43:52 skrll Exp $

.include <bsd.sys.mk>		# for HOST_SH

DEP=	syscalls.conf syscalls.master ../../kern/makesyscalls.sh
OBJS=	irix_sysent.c irix_syscalls.c irix_syscall.h irix_syscallargs.h

${OBJS}: ${DEP}
	${HOST_SH} ../../kern/makesyscalls.sh syscalls.conf syscalls.master
