#	$NetBSD: Makefile,v 1.1.26.1 2004/08/03 10:44:34 skrll Exp $

.include <bsd.sys.mk>		# for HOST_SH

DEP=	syscalls.conf syscalls.master ../../kern/makesyscalls.sh
OBJS=	svr4_32_sysent.c svr4_32_syscalls.c svr4_32_syscall.h svr4_32_syscallargs.h

#syscalls.master: ../svr4/syscalls.master sed.script
#	sed -f sed.script < ../svr4/syscalls.master > syscalls.master

${OBJS}: ${DEP}
	${HOST_SH} ../../kern/makesyscalls.sh syscalls.conf syscalls.master
