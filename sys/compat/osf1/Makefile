#	$NetBSD: Makefile,v 1.3 2003/10/26 07:25:38 lukem Exp $

.include <bsd.sys.mk>		# for HOST_SH

DEP=	syscalls.conf syscalls.master ../../kern/makesyscalls.sh
OBJS=	osf1_sysent.c osf1_syscalls.c osf1_syscall.h osf1_syscallargs.h

${OBJS}: ${DEP}
	-mv -f osf1_sysent.c osf1_sysent.c.bak
	-mv -f osf1_syscalls.c osf1_syscalls.c.bak
	-mv -f osf1_syscall.h osf1_syscall.h.bak
	-mv -f osf1_syscallargs.h osf1_syscallargs.h.bak
	${HOST_SH} ../../kern/makesyscalls.sh syscalls.conf syscalls.master

clean:
	rm -f ${OBJS}
