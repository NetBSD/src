#	$NetBSD: Makefile,v 1.3 2003/11/15 20:30:59 thorpej Exp $

.include <bsd.sys.mk>		# for HOST_SH

DEP=	syscalls.conf syscalls.master ../../kern/makesyscalls.sh
OBJS=	pecoff_sysent.c pecoff_syscalls.c pecoff_syscall.h pecoff_syscallargs.h

${OBJS}: ${DEP}
	${HOST_SH} ../../kern/makesyscalls.sh syscalls.conf syscalls.master
