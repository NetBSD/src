#	$NetBSD: Makefile,v 1.4 2003/11/15 20:30:59 thorpej Exp $

.include <bsd.sys.mk>		# for HOST_SH

DEP=	syscalls.conf syscalls.master ../../kern/makesyscalls.sh
OBJS=	netbsd32_sysent.c netbsd32_syscalls.c netbsd32_syscall.h netbsd32_syscallargs.h

${OBJS}: ${DEP}
	${HOST_SH} ../../kern/makesyscalls.sh syscalls.conf syscalls.master
