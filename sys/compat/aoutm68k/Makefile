#	$NetBSD: Makefile,v 1.1.2.2 2001/01/08 14:56:43 bouyer Exp $

DEP=	syscalls.conf syscalls.master ../../kern/makesyscalls.sh
OBJS=	aoutm68k_sysent.c aoutm68k_syscalls.c \
	aoutm68k_syscall.h aoutm68k_syscallargs.h

${OBJS}: ${DEP}
	-mv -f aoutm68k_sysent.c aoutm68k_sysent.c.bak
	-mv -f aoutm68k_syscalls.c aoutm68k_syscalls.c.bak
	-mv -f aoutm68k_syscall.h aoutm68k_syscall.h.bak
	-mv -f aoutm68k_syscallargs.h aoutm68k_syscallargs.h.bak
	sh ../../kern/makesyscalls.sh syscalls.conf syscalls.master
