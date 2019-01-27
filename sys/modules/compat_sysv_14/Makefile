# $NetBSD: Makefile,v 1.2 2019/01/27 02:08:47 pgoyette Exp $

.include "../Makefile.inc"

.PATH:	${S}/kern

KMOD=	compat_sysv_14

CPPFLAGS+=	-DSYSVSEM -DSYSVSHM -DSYSVMSG

.PATH:	${S}/compat/common

CPPFLAGS+=	-DCOMPAT_14
CPPFLAGS+=	-DCOMPAT_50

SRCS+=	compat_sysv_14_mod.c
SRCS+=	sysv_msg_14.c sysv_sem_14.c sysv_shm_14.c

.include <bsd.kmodule.mk>
