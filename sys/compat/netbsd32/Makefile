#	$NetBSD: Makefile,v 1.3 2003/10/26 07:25:37 lukem Exp $

.include <bsd.sys.mk>		# for HOST_SH

DEP=	syscalls.conf syscalls.master ../../kern/makesyscalls.sh
OBJS=	netbsd32_sysent.c netbsd32_syscalls.c netbsd32_syscall.h netbsd32_syscallargs.h

${OBJS}: ${DEP}
	-mv -f netbsd32_sysent.c netbsd32_sysent.c.bak
	-mv -f netbsd32_syscalls.c netbsd32_syscalls.c.bak
	-mv -f netbsd32_syscall.h netbsd32_syscall.h.bak
	-mv -f netbsd32_syscallargs.h netbsd32_syscallargs.h.bak
	${HOST_SH} ../../kern/makesyscalls.sh syscalls.conf syscalls.master
