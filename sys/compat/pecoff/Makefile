#	$NetBSD: Makefile,v 1.1.16.1 2004/08/03 10:44:24 skrll Exp $

.include <bsd.sys.mk>		# for HOST_SH

DEP=	syscalls.conf syscalls.master ../../kern/makesyscalls.sh
OBJS=	pecoff_sysent.c pecoff_syscalls.c pecoff_syscall.h pecoff_syscallargs.h

${OBJS}: ${DEP}
	${HOST_SH} ../../kern/makesyscalls.sh syscalls.conf syscalls.master
