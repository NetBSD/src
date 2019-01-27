#	$NetBSD: Makefile,v 1.2 2019/01/27 02:08:46 pgoyette Exp $

.include "../Makefile.inc"

.PATH:	${S}/compat/netbsd32

KMOD=	compat_netbsd32_60

CPPFLAGS+=	-DCOMPAT_NETBSD32
CPPFLAGS+=	-DCOMPAT_60 -DCOMPAT_70 -DCOMPAT_80
CPPFLAGS+=	-I${NETBSDSRCDIR}/sys/external/bsd/libnv/dist

SRCS+=	netbsd32_compat_60.c

.include <bsd.kmodule.mk>
