#	$NetBSD: Makefile,v 1.1 2002/11/03 23:17:20 manu Exp $

DEP=	syscalls.conf syscalls.master ../../../../../kern/makesyscalls.sh
OBJS=	mach_fasttraps_sysent.c mach_fasttraps_syscalls.c \
	mach_fasttraps_syscall.h mach_fasttraps_syscallargs.h

${OBJS}: ${DEP}
	-mv -f mach_fasttraps_sysent.c mach_fasttraps_sysent.c.bak
	-mv -f mach_fasttraps_syscalls.c mach_fasttraps_syscalls.c.bak
	-mv -f mach_fasttraps_syscall.h mach_fasttraps_syscall.h.bak
	-mv -f mach_fasttraps_syscallargs.h mach_fasttraps_syscallargs.h.bak
	sh ../../../../../kern/makesyscalls.sh syscalls.conf syscalls.master
