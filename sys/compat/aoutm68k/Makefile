#	$NetBSD: Makefile,v 1.2 2003/10/26 07:25:37 lukem Exp $

.include <bsd.sys.mk>		# for HOST_SH

DEP=	syscalls.conf syscalls.master ../../kern/makesyscalls.sh
OBJS=	aoutm68k_sysent.c aoutm68k_syscalls.c \
	aoutm68k_syscall.h aoutm68k_syscallargs.h

${OBJS}: ${DEP}
	-mv -f aoutm68k_sysent.c aoutm68k_sysent.c.bak
	-mv -f aoutm68k_syscalls.c aoutm68k_syscalls.c.bak
	-mv -f aoutm68k_syscall.h aoutm68k_syscall.h.bak
	-mv -f aoutm68k_syscallargs.h aoutm68k_syscallargs.h.bak
	${HOST_SH} ../../kern/makesyscalls.sh syscalls.conf syscalls.master
