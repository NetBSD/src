#	$NetBSD: Makefile,v 1.3.62.2 2004/09/18 14:43:15 skrll Exp $

.include <bsd.sys.mk>		# for HOST_SH

DEP=	syscalls.conf syscalls.master ../../kern/makesyscalls.sh
OBJS=	freebsd_sysent.c freebsd_syscalls.c \
	freebsd_syscall.h freebsd_syscallargs.h

${OBJS}: ${DEP}
	${HOST_SH} ../../kern/makesyscalls.sh syscalls.conf syscalls.master
