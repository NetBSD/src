#	$NetBSD: Makefile,v 1.2 2003/10/26 07:25:37 lukem Exp $

.include <bsd.sys.mk>		# for HOST_SH

DEP=	syscalls.conf syscalls.master ../../kern/makesyscalls.sh
OBJS=	darwin_sysent.c darwin_syscalls.c darwin_syscall.h darwin_syscallargs.h

${OBJS}: ${DEP}
	-mv -f darwin_sysent.c darwin_sysent.c.bak
	-mv -f darwin_syscalls.c darwin_syscalls.c.bak
	-mv -f darwin_syscall.h darwin_syscall.h.bak
	-mv -f darwin_syscallargs.h darwin_syscallargs.h.bak
	${HOST_SH} ../../kern/makesyscalls.sh syscalls.conf syscalls.master
