#	$NetBSD: Makefile,v 1.1 2001/02/02 07:28:54 mrg Exp $

DEP=	syscalls.conf syscalls.master ../../kern/makesyscalls.sh
OBJS=	sunos32_sysent.c sunos32_syscalls.c sunos32_syscall.h sunos32_syscallargs.h

all: ${OBJS} ${OBJS32}

${OBJS}: ${DEP}
	-mv -f sunos32_sysent.c sunos32_sysent.c.bak
	-mv -f sunos32_syscalls.c sunos32_syscalls.c.bak
	-mv -f sunos32_syscall.h sunos32_syscall.h.bak
	-mv -f sunos32_syscallargs.h sunos32_syscallargs.h.bak
	sh ../../kern/makesyscalls.sh syscalls.conf syscalls.master
