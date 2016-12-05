# $NetBSD: Makefile,v 1.1.2.4 2016/12/05 10:55:27 skrll Exp $

.include "../Makefile.inc"

.PATH:	${S}/kern

KMOD=	sysv_ipc

CPPFLAGS+=	-DSYSVSEM -DSYSVSHM -DSYSVMSG

SRCS+=	sysv_ipc.c sysv_msg.c sysv_sem.c sysv_shm.c

.PATH:	${S}/compat/common

CPPFLAGS+=	-DCOMPAT_10
CPPFLAGS+=	-DCOMPAT_13
CPPFLAGS+=	-DCOMPAT_14
CPPFLAGS+=	-DCOMPAT_50

.include "../../compat/common/Makefile.sysv"

.include <bsd.kmodule.mk>
