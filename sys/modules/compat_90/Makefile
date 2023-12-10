#	$NetBSD: Makefile,v 1.1.28.1 2023/12/10 13:06:16 martin Exp $

.include "../Makefile.inc"

.PATH:	${S}/compat/common

KMOD=	compat_90

CPPFLAGS+=	-DCOMPAT_90
CPPFLAGS+=	-DINET

# Remove/comment the following if the target kernel on which the
# module may be loaded doesn't have IPv6.  Enable this by default.

CPPFLAGS+=	-DINET6

SRCS+=	compat_90_mod.c
SRCS+=	net_inet6_nd_90.c
SRCS+=	vfs_syscalls_90.c

.include <bsd.kmodule.mk>
