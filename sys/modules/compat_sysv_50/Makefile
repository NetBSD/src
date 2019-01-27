# $NetBSD: Makefile,v 1.2 2019/01/27 02:08:47 pgoyette Exp $

.include "../Makefile.inc"

.PATH:	${S}/kern

KMOD=	compat_sysv_50

CPPFLAGS+=	-DSYSVSEM -DSYSVSHM -DSYSVMSG

.PATH:	${S}/compat/common

CPPFLAGS+=	-DCOMPAT_50

SRCS+=	compat_sysv_50_mod.c
SRCS+=	sysv_ipc_50.c sysv_msg_50.c sysv_sem_50.c sysv_shm_50.c

.include <bsd.kmodule.mk>
