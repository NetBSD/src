#	$NetBSD: Makefile,v 1.2 1999/03/25 16:22:49 mrg Exp $

DEP=	syscalls.conf syscalls.master ../../kern/makesyscalls.sh
OBJS=	netbsd32_sysent.c netbsd32_syscalls.c netbsd32_syscall.h netbsd32_syscallargs.h

${OBJS}: ${DEP}
	-mv -f netbsd32_sysent.c netbsd32_sysent.c.bak
	-mv -f netbsd32_syscalls.c netbsd32_syscalls.c.bak
	-mv -f netbsd32_syscall.h netbsd32_syscall.h.bak
	-mv -f netbsd32_syscallargs.h netbsd32_syscallargs.h.bak
	sh ../../kern/makesyscalls.sh syscalls.conf syscalls.master
