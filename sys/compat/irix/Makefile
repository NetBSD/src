#	$NetBSD: Makefile,v 1.2 2003/10/26 07:25:37 lukem Exp $

.include <bsd.sys.mk>		# for HOST_SH

DEP=	syscalls.conf syscalls.master ../../kern/makesyscalls.sh
OBJS=	irix_sysent.c irix_syscalls.c irix_syscall.h irix_syscallargs.h

${OBJS}: ${DEP}
	-mv -f irix_sysent.c irix_sysent.c.bak
	-mv -f irix_syscalls.c irix_syscalls.c.bak
	-mv -f irix_syscall.h irix_syscall.h.bak
	-mv -f irix_syscallargs.h irix_syscallargs.h.bak
	${HOST_SH} ../../kern/makesyscalls.sh syscalls.conf syscalls.master
