# $NetBSD: Makefile,v 1.3.10.4 2018/03/10 02:18:56 pgoyette Exp $

.include "../Makefile.inc"

.PATH:	${S}/kern

KMOD=	compat_sysv

CPPFLAGS+=	-DSYSVSEM -DSYSVSHM -DSYSVMSG

.PATH:	${S}/compat/common

CPPFLAGS+=	-DCOMPAT_10
CPPFLAGS+=	-DCOMPAT_13
CPPFLAGS+=	-DCOMPAT_14
CPPFLAGS+=	-DCOMPAT_50

SRCS+=	sysv_mod.c
SRCS+=	kern_ipc_10.c sysv_ipc_50.c
SRCS+=	sysv_msg_14.c sysv_sem_14.c sysv_shm_14.c
SRCS+=	sysv_msg_50.c sysv_sem_50.c sysv_shm_50.c

.include <bsd.kmodule.mk>
