#	$NetBSD: Makefile,v 1.1 2002/03/18 07:11:06 oki Exp $

DEP=	syscalls.conf syscalls.master ../../kern/makesyscalls.sh
OBJS=	pecoff_sysent.c pecoff_syscalls.c pecoff_syscall.h pecoff_syscallargs.h

${OBJS}: ${DEP}
	-mv -f pecoff_sysent.c pecoff_sysent.c.bak
	-mv -f pecoff_syscalls.c pecoff_syscalls.c.bak
	-mv -f pecoff_syscall.h pecoff_syscall.h.bak
	-mv -f pecoff_syscallargs.h pecoff_syscallargs.h.bak
	sh ../../kern/makesyscalls.sh syscalls.conf syscalls.master
