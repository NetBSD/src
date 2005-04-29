#	$NetBSD: Makefile,v 1.3.8.1 2005/04/29 11:28:32 kent Exp $

.include <bsd.sys.mk>		# for HOST_SH

DEP=	syscalls.conf syscalls.master ../../kern/makesyscalls.sh
OBJS=	ibcs2_sysent.c ibcs2_syscalls.c ibcs2_syscall.h ibcs2_syscallargs.h

${OBJS}: ${DEP}
	${HOST_SH} ../../kern/makesyscalls.sh syscalls.conf syscalls.master

all: ${OBJS}

.include <bsd.kinc.mk>
