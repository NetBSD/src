#	$NetBSD: Makefile.sc,v 1.1 1998/09/30 21:38:46 erh Exp $

DEP=	syscalls.conf syscalls.master ../../../kern/makesyscalls.sh
OBJS=	linux_sysent.c linux_syscalls.c linux_syscall.h linux_syscallargs.h

${OBJS}: ${DEP}
	-mv -f linux_sysent.c linux_sysent.c.bak
	-mv -f linux_syscalls.c linux_syscalls.c.bak
	-mv -f linux_syscall.h linux_syscall.h.bak
	-mv -f linux_syscallargs.h linux_syscallargs.h.bak
	sh ../../../kern/makesyscalls.sh syscalls.conf syscalls.master

all: ${OBJS}
