#	$NetBSD: Makefile,v 1.2 2003/10/26 07:25:38 lukem Exp $

.include <bsd.sys.mk>		# for HOST_SH

DEP=	syscalls.conf syscalls.master ../../kern/makesyscalls.sh
OBJS=	sunos_sysent.c sunos_syscalls.c sunos_syscall.h sunos_syscallargs.h

${OBJS}: ${DEP}
	-mv -f sunos_sysent.c sunos_sysent.c.bak
	-mv -f sunos_syscalls.c sunos_syscalls.c.bak
	-mv -f sunos_syscall.h sunos_syscall.h.bak
	-mv -f sunos_syscallargs.h sunos_syscallargs.h.bak
	${HOST_SH} ../../kern/makesyscalls.sh syscalls.conf syscalls.master
