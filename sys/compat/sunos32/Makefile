#	$NetBSD: Makefile,v 1.3 2003/11/15 20:31:00 thorpej Exp $

.include <bsd.sys.mk>		# for HOST_SH

DEP=	syscalls.conf syscalls.master ../../kern/makesyscalls.sh
OBJS=	sunos32_sysent.c sunos32_syscalls.c sunos32_syscall.h sunos32_syscallargs.h

all: ${OBJS} ${OBJS32}

${OBJS}: ${DEP}
	${HOST_SH} ../../kern/makesyscalls.sh syscalls.conf syscalls.master
