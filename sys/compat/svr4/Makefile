#	$NetBSD: Makefile,v 1.2 2003/10/26 07:25:38 lukem Exp $

.include <bsd.sys.mk>		# for HOST_SH

DEP=	syscalls.conf syscalls.master ../../kern/makesyscalls.sh
OBJS=	svr4_sysent.c svr4_syscalls.c svr4_syscall.h svr4_syscallargs.h

${OBJS}: ${DEP}
	-mv -f svr4_sysent.c svr4_sysent.c.bak
	-mv -f svr4_syscalls.c svr4_syscalls.c.bak
	-mv -f svr4_syscall.h svr4_syscall.h.bak
	-mv -f svr4_syscallargs.h svr4_syscallargs.h.bak
	${HOST_SH} ../../kern/makesyscalls.sh syscalls.conf syscalls.master
