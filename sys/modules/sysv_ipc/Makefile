# $NetBSD: Makefile,v 1.1 2015/05/13 02:06:25 pgoyette Exp $

.include "../Makefile.inc"

.PATH:	${S}/kern

KMOD=	sysv_ipc

CPPFLAGS+=	-DSYSVSEM -DSYSVSHM -DSYSVMSG

SRCS=	sysv_ipc.c sysv_msg.c sysv_sem.c sysv_shm.c

.include <bsd.kmodule.mk>
