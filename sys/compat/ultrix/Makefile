#	$NetBSD: Makefile,v 1.2 2003/10/26 07:25:38 lukem Exp $

.include <bsd.sys.mk>		# for HOST_SH

DEP=	syscalls.conf syscalls.master ../../kern/makesyscalls.sh
OBJS=	ultrix_sysent.c ultrix_syscalls.c ultrix_syscall.h ultrix_syscallargs.h

${OBJS}: ${DEP}
	-mv -f ultrix_sysent.c ultrix_sysent.c.bak
	-mv -f ultrix_syscalls.c ultrix_syscalls.c.bak
	-mv -f ultrix_syscall.h ultrix_syscall.h.bak
	-mv -f ultrix_syscallargs.h ultrix_syscallargs.h.bak
	${HOST_SH} ../../kern/makesyscalls.sh syscalls.conf syscalls.master
