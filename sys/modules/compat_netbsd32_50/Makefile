#	$NetBSD: Makefile,v 1.1.2.2 2018/09/11 04:20:51 pgoyette Exp $

.include "../Makefile.inc"

.PATH:	${S}/compat/netbsd32

KMOD=	compat_netbsd32_50

CPPFLAGS+=	-DCOMPAT_NETBSD32
CPPFLAGS+=	-DCOMPAT_50 -DCOMPAT_60 -DCOMPAT_70 -DCOMPAT_80

SRCS+=	netbsd32_compat_50.c

.include <bsd.kmodule.mk>
