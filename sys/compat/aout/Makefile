#	$NetBSD: Makefile,v 1.1 1999/02/15 08:48:20 christos Exp $

DEP=	syscalls.conf syscalls.master ../../kern/makesyscalls.sh
OBJS=	aout_sysent.c aout_syscalls.c aout_syscall.h aout_syscallargs.h

${OBJS}: ${DEP}
	-mv -f aout_sysent.c aout_sysent.c.bak
	-mv -f aout_syscalls.c aout_syscalls.c.bak
	-mv -f aout_syscall.h aout_syscall.h.bak
	-mv -f aout_syscallargs.h aout_syscallargs.h.bak
	sh ../../kern/makesyscalls.sh syscalls.conf syscalls.master
