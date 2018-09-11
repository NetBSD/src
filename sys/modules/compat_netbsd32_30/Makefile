#	$NetBSD: Makefile,v 1.1.2.3 2018/09/11 23:58:46 pgoyette Exp $

.include "../Makefile.inc"

.PATH:	${S}/compat/netbsd32

KMOD=	compat_netbsd32_30

CPPFLAGS+=	-DNTP
CPPFLAGS+=	-DCOMPAT_NETBSD32
CPPFLAGS+=	-DCOMPAT_30
CPPFLAGS+=	-DCOMPAT_40 -DCOMPAT_50 -DCOMPAT_60 -DCOMPAT_70 -DCOMPAT_80

SRCS+=	netbsd32_compat_30.c

.include <bsd.kmodule.mk>
