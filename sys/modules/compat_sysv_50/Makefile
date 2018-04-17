# $NetBSD: Makefile,v 1.1.2.1 2018/04/17 23:06:12 pgoyette Exp $

.include "../Makefile.inc"

.PATH:	${S}/kern

KMOD=	compat_sysv_50

CPPFLAGS+=	-DSYSVSEM -DSYSVSHM -DSYSVMSG

.PATH:	${S}/compat/common

CPPFLAGS+=	-DCOMPAT_50

SRCS+=	compat_sysv_50_mod.c
SRCS+=	sysv_ipc_50.c sysv_msg_50.c sysv_sem_50.c sysv_shm_50.c

.include <bsd.kmodule.mk>
