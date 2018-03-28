#	$NetBSD: Makefile,v 1.1.2.2 2018/03/28 07:51:09 pgoyette Exp $

.include "../Makefile.inc"

.PATH:	${S}/compat/common

KMOD=	compat_30

CPPFLAGS+=	-DCOMPAT_30
CPPFLAGS+=	-DCOMPAT_40 -DCOMPAT_50 -DCOMPAT_60 -DCOMPAT_70 -DCOMPAT_80

SRCS+=	compat_30_mod.c
SRCS+=	vfs_syscalls_30.c uipc_syscalls_30.c kern_time_30.c bio_30.c vnd_30.c

.include <bsd.kmodule.mk>
