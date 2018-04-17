# $NetBSD: Makefile,v 1.1.2.1 2018/04/17 23:06:11 pgoyette Exp $

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
