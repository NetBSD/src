#	$NetBSD: Makefile,v 1.1 2019/09/22 22:59:39 christos Exp $

.include "../Makefile.inc"

.PATH:	${S}/compat/common

KMOD=	compat_90

CPPFLAGS+=	-DCOMPAT_90

SRCS+=	compat_90_mod.c
SRCS+=	vfs_syscalls_90.c

.include <bsd.kmodule.mk>
