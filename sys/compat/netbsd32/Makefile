#	$NetBSD: Makefile,v 1.2.42.1 2004/08/03 10:44:21 skrll Exp $

.include <bsd.sys.mk>		# for HOST_SH

DEP=	syscalls.conf syscalls.master ../../kern/makesyscalls.sh
OBJS=	netbsd32_sysent.c netbsd32_syscalls.c netbsd32_syscall.h netbsd32_syscallargs.h

${OBJS}: ${DEP}
	${HOST_SH} ../../kern/makesyscalls.sh syscalls.conf syscalls.master
