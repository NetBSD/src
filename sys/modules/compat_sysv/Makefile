#	$NetBSD: Makefile,v 1.1.2.3 2015/09/22 12:06:09 skrll Exp $

.include "../Makefile.inc"

.PATH:	${S}/compat/common

KMOD=	compat_sysv

CPPFLAGS+=	-DCOMPAT_10
CPPFLAGS+=	-DCOMPAT_13
CPPFLAGS+=	-DCOMPAT_14
CPPFLAGS+=	-DCOMPAT_50
CPPFLAGS+=	-DSYSVSEM -DSYSVSHM -DSYSVMSG

SRCS=	compat_sysv_mod.c
SRCS+=	sysv_msg_14.c sysv_sem_14.c sysv_shm_14.c
SRCS+=	sysv_msg_50.c sysv_sem_50.c sysv_shm_50.c sysv_ipc_50.c

.if ${MACHINE_ARCH} != "alpha"
SRCS+=  kern_ipc_10.c
.endif

.include <bsd.kmodule.mk>
