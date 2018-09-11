#	$NetBSD: Makefile,v 1.1.2.1 2018/09/11 07:01:10 pgoyette Exp $

.include "../Makefile.inc"

.PATH:	${S}/compat/netbsd32

KMOD=	compat_netbsd32_80

CPPFLAGS+=	-DCOMPAT_NETBSD32
CPPFLAGS+=	-DCOMPAT_80

SRCS+=	netbsd32_compat_80.c

.include <bsd.kmodule.mk>
