#	$NetBSD: Makefile,v 1.1 2000/12/02 20:40:03 scw Exp $

DEP=	syscalls.conf syscalls.master ../../kern/makesyscalls.sh
OBJS=	aoutm68k_sysent.c aoutm68k_syscalls.c \
	aoutm68k_syscall.h aoutm68k_syscallargs.h

${OBJS}: ${DEP}
	-mv -f aoutm68k_sysent.c aoutm68k_sysent.c.bak
	-mv -f aoutm68k_syscalls.c aoutm68k_syscalls.c.bak
	-mv -f aoutm68k_syscall.h aoutm68k_syscall.h.bak
	-mv -f aoutm68k_syscallargs.h aoutm68k_syscallargs.h.bak
	sh ../../kern/makesyscalls.sh syscalls.conf syscalls.master
