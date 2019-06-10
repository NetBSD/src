# $NetBSD: Makefile,v 1.3.18.1 2019/06/10 22:09:40 christos Exp $

.include "../Makefile.inc"

.PATH:	${S}/kern

KMOD=	sysv_ipc

CPPFLAGS+=	-DSYSVSEM -DSYSVSHM -DSYSVMSG

SRCS+=	sysv_ipc.c sysv_msg.c sysv_sem.c sysv_shm.c

WARNS=	3

.include <bsd.kmodule.mk>
