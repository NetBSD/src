#	$NetBSD: Makefile,v 1.1.8.1 2004/08/03 10:43:29 skrll Exp $

.include <bsd.sys.mk>		# for HOST_SH

DEP=	syscalls.conf syscalls.master ../../kern/makesyscalls.sh
OBJS=	darwin_sysent.c darwin_syscalls.c darwin_syscall.h darwin_syscallargs.h

${OBJS}: ${DEP}
	${HOST_SH} ../../kern/makesyscalls.sh syscalls.conf syscalls.master
