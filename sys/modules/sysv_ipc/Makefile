# $NetBSD: Makefile,v 1.2 2015/12/03 02:51:01 pgoyette Exp $

.include "../Makefile.inc"

.PATH:	${S}/kern

KMOD=	sysv_ipc

CPPFLAGS+=	-DSYSVSEM -DSYSVSHM -DSYSVMSG

SRCS+=	sysv_ipc.c sysv_msg.c sysv_sem.c sysv_shm.c

.PATH:	${S}/compat/common

CPPFLAGS+=	-DCOMPAT_10
CPPFLAGS+=	-DCOMPAT_13
CPPFLAGS+=	-DCOMPAT_14
CPPFLAGS+=	-DCOMPAT_50

SRCS+=	sysv_msg_14.c sysv_sem_14.c sysv_shm_14.c
SRCS+=	sysv_msg_50.c sysv_sem_50.c sysv_shm_50.c sysv_ipc_50.c

.if ${MACHINE_ARCH} != "alpha"
SRCS+=  kern_ipc_10.c
.endif

.include <bsd.kmodule.mk>
