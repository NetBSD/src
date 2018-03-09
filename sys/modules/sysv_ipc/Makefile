# $NetBSD: Makefile,v 1.3.16.1 2018/03/09 01:27:50 pgoyette Exp $

.include "../Makefile.inc"

.PATH:	${S}/kern

KMOD=	sysv_ipc

CPPFLAGS+=	-DSYSVSEM -DSYSVSHM -DSYSVMSG

SRCS+=	sysv_ipc.c sysv_msg.c sysv_sem.c sysv_shm.c

.include <bsd.kmodule.mk>
