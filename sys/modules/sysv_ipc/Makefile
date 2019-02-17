# $NetBSD: Makefile,v 1.5 2019/02/17 04:05:58 rin Exp $

.include "../Makefile.inc"

.PATH:	${S}/kern

KMOD=	sysv_ipc

CPPFLAGS+=	-DSYSVSEM -DSYSVSHM -DSYSVMSG

SRCS+=	sysv_ipc.c sysv_msg.c sysv_sem.c sysv_shm.c

WARNS=	3

.include <bsd.kmodule.mk>
