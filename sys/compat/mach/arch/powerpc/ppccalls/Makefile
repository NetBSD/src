#	$NetBSD: Makefile,v 1.1 2002/11/03 23:17:22 manu Exp $

DEP=	syscalls.conf syscalls.master ../../../../../kern/makesyscalls.sh
OBJS=	mach_ppccalls_sysent.c mach_ppccalls_syscalls.c \
	mach_ppccalls_syscall.h mach_ppccalls_syscallargs.h

${OBJS}: ${DEP}
	-mv -f mach_ppccalls_sysent.c mach_ppccalls_sysent.c.bak
	-mv -f mach_ppccalls_syscalls.c mach_ppccalls_syscalls.c.bak
	-mv -f mach_ppccalls_syscall.h mach_ppccalls_syscall.h.bak
	-mv -f mach_ppccalls_syscallargs.h mach_ppccalls_syscallargs.h.bak
	sh ../../../../../kern/makesyscalls.sh syscalls.conf syscalls.master
