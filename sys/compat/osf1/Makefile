#	$NetBSD: Makefile,v 1.2.36.3 2004/09/21 13:25:58 skrll Exp $

.include <bsd.sys.mk>		# for HOST_SH

DEP=	syscalls.conf syscalls.master ../../kern/makesyscalls.sh
OBJS=	osf1_sysent.c osf1_syscalls.c osf1_syscall.h osf1_syscallargs.h

${OBJS}: ${DEP}
	${HOST_SH} ../../kern/makesyscalls.sh syscalls.conf syscalls.master

clean:
	rm -f ${OBJS}
