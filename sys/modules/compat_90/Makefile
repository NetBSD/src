#	$NetBSD: Makefile,v 1.2 2023/12/09 15:21:01 pgoyette Exp $

.include "../Makefile.inc"

.PATH:	${S}/compat/common

KMOD=	compat_90

CPPFLAGS+=	-DCOMPAT_90

SRCS+=	compat_90_mod.c
SRCS+=	net_inet6_nd_90.c
SRCS+=	vfs_syscalls_90.c

.include <bsd.kmodule.mk>
