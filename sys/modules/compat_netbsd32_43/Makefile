#	$NetBSD: Makefile,v 1.1.2.2 2018/09/11 08:41:27 pgoyette Exp $

.include "../Makefile.inc"

.PATH:	${S}/compat/netbsd32

KMOD=	compat_netbsd32_43

CPPFLAGS+=	-DCOMPAT_NETBSD32
CPPFLAGS+=	-DCOMPAT_43

SRCS+=	netbsd32_compat_43.c

.include <bsd.kmodule.mk>
