#	$NetBSD: Makefile,v 1.1 1998/08/26 10:20:33 mrg Exp $

DEP=	syscalls.conf syscalls.master ../../kern/makesyscalls.sh
OBJS=	sparc32_sysent.c sparc32_syscalls.c sparc32_syscall.h sparc32_syscallargs.h

${OBJS}: ${DEP}
	-mv -f sparc32_sysent.c sparc32_sysent.c.bak
	-mv -f sparc32_syscalls.c sparc32_syscalls.c.bak
	-mv -f sparc32_syscall.h sparc32_syscall.h.bak
	-mv -f sparc32_syscallargs.h sparc32_syscallargs.h.bak
	sh ../../kern/makesyscalls.sh syscalls.conf syscalls.master
