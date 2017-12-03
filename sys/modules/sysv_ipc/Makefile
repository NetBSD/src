# $NetBSD: Makefile,v 1.3.14.2 2017/12/03 11:39:00 jdolecek Exp $

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
