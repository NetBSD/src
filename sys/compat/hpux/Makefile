#	$NetBSD: Makefile,v 1.4 1996/09/03 03:18:07 mycroft Exp $

DEP=	syscalls.conf syscalls.master ../../kern/makesyscalls.sh
OBJS=	hpux_sysent.c hpux_syscalls.c hpux_syscall.h hpux_syscallargs.h

${OBJS}: ${DEP}
	-mv -f hpux_sysent.c hpux_sysent.c.bak
	-mv -f hpux_syscalls.c hpux_syscalls.c.bak
	-mv -f hpux_syscall.h hpux_syscall.h.bak
	-mv -f hpux_syscallargs.h hpux_syscallargs.h.bak
	sh ../../kern/makesyscalls.sh syscalls.conf syscalls.master
