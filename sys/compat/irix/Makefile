#	$NetBSD: Makefile,v 1.1.8.2 2002/01/10 19:51:16 thorpej Exp $

DEP=	syscalls.conf syscalls.master ../../kern/makesyscalls.sh
OBJS=	irix_sysent.c irix_syscalls.c irix_syscall.h irix_syscallargs.h

${OBJS}: ${DEP}
	-mv -f irix_sysent.c irix_sysent.c.bak
	-mv -f irix_syscalls.c irix_syscalls.c.bak
	-mv -f irix_syscall.h irix_syscall.h.bak
	-mv -f irix_syscallargs.h irix_syscallargs.h.bak
	sh ../../kern/makesyscalls.sh syscalls.conf syscalls.master
