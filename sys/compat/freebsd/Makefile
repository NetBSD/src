#	$NetBSD: Makefile,v 1.3 1996/12/10 08:54:53 mycroft Exp $

DEP=	syscalls.conf syscalls.master ../../kern/makesyscalls.sh
OBJS=	freebsd_sysent.c freebsd_syscalls.c \
	freebsd_syscall.h freebsd_syscallargs.h

${OBJS}: ${DEP}
	-mv -f freebsd_sysent.c freebsd_sysent.c.bak
	-mv -f freebsd_syscalls.c freebsd_syscalls.c.bak
	-mv -f freebsd_syscall.h freebsd_syscall.h.bak
	-mv -f freebsd_syscallargs.h freebsd_syscallargs.h.bak
	sh ../../kern/makesyscalls.sh syscalls.conf syscalls.master
