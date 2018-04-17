# $NetBSD: Makefile,v 1.1.2.1 2018/04/17 23:06:12 pgoyette Exp $

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
