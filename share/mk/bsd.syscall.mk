# $NetBSD: bsd.syscall.mk,v 1.1 2014/01/14 18:51:45 christos Exp $
#
.include <bsd.own.mk>
.include <bsd.sys.mk>

SYSCALL_OBJS?=	${SYSCALL_PREFIX}_sysent.c ${SYSCALL_PREFIX}_syscalls.c \
	${SYSCALL_PREFIX}_syscall.h ${SYSCALL_PREFIX}_syscallargs.h

SYSCALL_DEPS?=	${NETBSDSRCDIR}/sys/kern/makesyscalls.sh \
	syscalls.conf syscalls.master

all: ${SYSCALL_OBJS}

${SYSCALL_OBJS}: ${SYSCALL_DEPS}
	${HOST_SH} ${.ALLSRC}

.include <bsd.kinc.mk>
