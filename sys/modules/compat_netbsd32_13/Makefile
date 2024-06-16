#	$NetBSD: Makefile,v 1.5 2024/06/16 21:52:02 pgoyette Exp $

.include <bsd.init.mk>

.PATH:	${S}/compat/netbsd32

KMOD=	compat_netbsd32_13

CPPFLAGS+=	-DCOMPAT_NETBSD32
CPPFLAGS+=	-DCOMPAT_13 -DCOMPAT_14 -DCOMPAT_16 -DCOMPAT_20 -DCOMPAT_30
CPPFLAGS+=	-DCOMPAT_40 -DCOMPAT_50 -DCOMPAT_60 -DCOMPAT_70 -DCOMPAT_80
CPPFLAGS+=	-I${NETBSDSRCDIR}/sys/external/bsd/libnv/dist

SRCS+=	netbsd32_compat_13.c

COMPAT_MACHDEP_SRCS=	netbsd32_machdep_13.c

.include "../Makefile.compat"

.include <bsd.kmodule.mk>
