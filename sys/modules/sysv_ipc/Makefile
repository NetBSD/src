# $NetBSD: Makefile,v 1.1.2.2 2015/06/06 14:40:25 skrll Exp $

.include "../Makefile.inc"

.PATH:	${S}/kern

KMOD=	sysv_ipc

CPPFLAGS+=	-DSYSVSEM -DSYSVSHM -DSYSVMSG

SRCS=	sysv_ipc.c sysv_msg.c sysv_sem.c sysv_shm.c

.include <bsd.kmodule.mk>
