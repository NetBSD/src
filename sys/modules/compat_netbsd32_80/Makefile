#	$NetBSD: Makefile,v 1.1.2.1 2018/09/10 22:50:52 pgoyette Exp $

.include "../Makefile.inc"

.PATH:	${S}/compat/netbsd32

KMOD=	compat_netbsd32_60

CPPFLAGS+=	-DCOMPAT_80

SRCS+=	netbsd32_compat_80.c

.include <bsd.kmodule.mk>
