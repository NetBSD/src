#	$NetBSD: Makefile,v 1.2 1994/10/26 02:45:11 cgd Exp $

hpux_sysent.c hpux_syscalls.c hpux_syscall.h: makesyscalls.sh syscalls.master
	-mv -f hpux_sysent.c hpux_sysent.c.bak
	-mv -f hpux_syscalls.c hpux_syscalls.c.bak
	-mv -f hpux_syscall.h hpux_syscall.h.bak
	sh makesyscalls.sh syscalls.master
