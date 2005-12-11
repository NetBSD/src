#	$NetBSD: Makefile,v 1.5 2005/12/11 12:20:21 christos Exp $

.include <bsd.sys.mk>		# for HOST_SH

DEP=	syscalls.conf syscalls.master ../../../../../kern/makesyscalls.sh
OBJS=	mach_fasttraps_sysent.c mach_fasttraps_syscalls.c \
	mach_fasttraps_syscall.h mach_fasttraps_syscallargs.h

${OBJS}: ${DEP}
	${HOST_SH} ../../../../../kern/makesyscalls.sh syscalls.conf syscalls.master

all: ${OBJS}

.include <bsd.kinc.mk>
