#	$NetBSD: Makefile,v 1.1.42.3 2004/09/21 13:24:55 skrll Exp $

.include <bsd.sys.mk>		# for HOST_SH

DEP=	syscalls.conf syscalls.master ../../kern/makesyscalls.sh
OBJS=	aout_sysent.c aout_syscalls.c aout_syscall.h aout_syscallargs.h

${OBJS}: ${DEP}
	${HOST_SH} ../../kern/makesyscalls.sh syscalls.conf syscalls.master
