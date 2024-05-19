#	$NetBSD: Makefile,v 1.1 2024/05/19 22:25:49 christos Exp $

.include "../Makefile.inc"

.PATH:	${S}/compat/netbsd32

KMOD=	compat_netbsd32_110

CPPFLAGS+=	-DCOMPAT_NETBSD32
CPPFLAGS+=	-DCOMPAT_110

SRCS+=	netbsd32_compat_110.c

.include <bsd.kmodule.mk>
