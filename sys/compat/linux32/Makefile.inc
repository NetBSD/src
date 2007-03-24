#	$NetBSD: Makefile.inc,v 1.1.28.1 2007/03/24 14:55:10 yamt Exp $

.include <bsd.sys.mk>		# for HOST_SH

DEP=	syscalls.conf syscalls.master ../../../../kern/makesyscalls.sh
OBJS=	linux32_sysent.c linux32_syscalls.c linux32_syscall.h linux32_syscallargs.h

${OBJS}: ${DEP}
	${HOST_SH} ../../../../kern/makesyscalls.sh syscalls.conf syscalls.master

all: ${OBJS}
