#	$NetBSD: Makefile,v 1.3 2003/11/15 20:30:59 thorpej Exp $

.include <bsd.sys.mk>		# for HOST_SH

DEP=	syscalls.conf syscalls.master ../../kern/makesyscalls.sh
OBJS=	aout_sysent.c aout_syscalls.c aout_syscall.h aout_syscallargs.h

${OBJS}: ${DEP}
	${HOST_SH} ../../kern/makesyscalls.sh syscalls.conf syscalls.master
