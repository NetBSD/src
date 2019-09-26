#	$NetBSD: Makefile,v 1.4 2019/09/26 02:01:03 christos Exp $

.include "../Makefile.inc"

.PATH:	${S}/compat/netbsd32

KMOD=	compat_netbsd32_20

CPPFLAGS+=	-DCOMPAT_NETBSD32
CPPFLAGS+=	-DCOMPAT_20 -DCOMPAT_30
CPPFLAGS+=	-DCOMPAT_40 -DCOMPAT_50 -DCOMPAT_60 -DCOMPAT_70 -DCOMPAT_80
CPPFLAGS+=	-I${NETBSDSRCDIR}/sys/external/bsd/libnv/dist

SRCS+=	netbsd32_compat_20.c

.include <bsd.kmodule.mk>
