#	$NetBSD: Makefile,v 1.1.4.2 2001/08/24 00:08:51 nathanw Exp $

DEP=	syscalls.conf syscalls.master ../../kern/makesyscalls.sh
OBJS=	mach_sysent.c mach_syscalls.c mach_syscall.h mach_syscallargs.h

${OBJS}: ${DEP}
	-mv -f mach_sysent.c mach_sysent.c.bak
	-mv -f mach_syscalls.c mach_syscalls.c.bak
	-mv -f mach_syscall.h mach_syscall.h.bak
	-mv -f mach_syscallargs.h mach_syscallargs.h.bak
	sh ../../kern/makesyscalls.sh syscalls.conf syscalls.master
