#	$NetBSD: Makefile,v 1.1.64.1 2004/08/03 10:43:46 skrll Exp $

.include <bsd.sys.mk>		# for HOST_SH

DEP=	syscalls.conf syscalls.master ../../kern/makesyscalls.sh
OBJS=	ibcs2_sysent.c ibcs2_syscalls.c ibcs2_syscall.h ibcs2_syscallargs.h

${OBJS}: ${DEP}
	${HOST_SH} ../../kern/makesyscalls.sh syscalls.conf syscalls.master
