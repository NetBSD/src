#	$NetBSD: Makefile,v 1.1.8.2 2020/04/13 08:05:07 martin Exp $

.include "../Makefile.inc"

.PATH:	${S}/compat/common

KMOD=	compat_90

CPPFLAGS+=	-DCOMPAT_90

SRCS+=	compat_90_mod.c
SRCS+=	vfs_syscalls_90.c

.include <bsd.kmodule.mk>
