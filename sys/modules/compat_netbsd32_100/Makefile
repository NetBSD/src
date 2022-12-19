#	$NetBSD: Makefile,v 1.1 2022/12/19 23:19:51 pgoyette Exp $

.include "../Makefile.inc"

.PATH:	${S}/compat/netbsd32

KMOD=	compat_netbsd32_100

CPPFLAGS+=	-DCOMPAT_NETBSD32
CPPFLAGS+=	-DCOMPAT_100
CPPFLAGS+=	-I${NETBSDSRCDIR}/sys/external/bsd/libnv/dist

SRCS+=	netbsd32_compat_100.c

.include <bsd.kmodule.mk>
