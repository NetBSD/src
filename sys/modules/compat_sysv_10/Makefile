# $NetBSD: Makefile,v 1.2.4.2 2019/06/10 22:09:19 christos Exp $

.include "../Makefile.inc"

.PATH:	${S}/kern

KMOD=	compat_sysv_10

CPPFLAGS+=	-DSYSVSEM -DSYSVSHM -DSYSVMSG

.PATH:	${S}/compat/common

CPPFLAGS+=	-DCOMPAT_10
CPPFLAGS+=	-DCOMPAT_13
CPPFLAGS+=	-DCOMPAT_14
CPPFLAGS+=	-DCOMPAT_50

SRCS+=	compat_sysv_10_mod.c
SRCS+=	kern_ipc_10.c

.include <bsd.kmodule.mk>
