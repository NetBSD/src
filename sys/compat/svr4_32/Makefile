#	$NetBSD: Makefile,v 1.1 2001/02/06 16:37:56 eeh Exp $

DEP=	syscalls.conf syscalls.master ../../kern/makesyscalls.sh
OBJS=	svr4_32_sysent.c svr4_32_syscalls.c svr4_32_syscall.h svr4_32_syscallargs.h

#syscalls.master: ../svr4/syscalls.master sed.script
#	sed -f sed.script < ../svr4/syscalls.master > syscalls.master

${OBJS}: ${DEP}
	-mv -f svr4_32_sysent.c svr4_32_sysent.c.bak
	-mv -f svr4_32_syscalls.c svr4_32_syscalls.c.bak
	-mv -f svr4_32_syscall.h svr4_32_syscall.h.bak
	-mv -f svr4_32_syscallargs.h svr4_32_syscallargs.h.bak
	sh ../../kern/makesyscalls.sh syscalls.conf syscalls.master
