#	$NetBSD: Makefile,v 1.2 2003/10/26 07:25:37 lukem Exp $

.include <bsd.sys.mk>		# for HOST_SH

DEP=	syscalls.conf syscalls.master ../../kern/makesyscalls.sh
OBJS=	aout_sysent.c aout_syscalls.c aout_syscall.h aout_syscallargs.h

${OBJS}: ${DEP}
	-mv -f aout_sysent.c aout_sysent.c.bak
	-mv -f aout_syscalls.c aout_syscalls.c.bak
	-mv -f aout_syscall.h aout_syscall.h.bak
	-mv -f aout_syscallargs.h aout_syscallargs.h.bak
	${HOST_SH} ../../kern/makesyscalls.sh syscalls.conf syscalls.master
