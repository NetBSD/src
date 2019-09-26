#	$NetBSD: Makefile,v 1.4 2019/09/26 02:01:02 christos Exp $

.include "../Makefile.inc"

.PATH:	${S}/compat/netbsd32

KMOD=	compat_netbsd32_09

CPPFLAGS+=	-DCOMPAT_NETBSD32
CPPFLAGS+=	-DCOMPAT_09 -DCOMPAT_10 -DCOMPAT_12
CPPFLAGS+=	-DCOMPAT_13 -DCOMPAT_14 -DCOMPAT_16 -DCOMPAT_20 -DCOMPAT_30
CPPFLAGS+=	-DCOMPAT_40 -DCOMPAT_50 -DCOMPAT_60 -DCOMPAT_70 -DCOMPAT_80
CPPFLAGS+=	-I${NETBSDSRCDIR}/sys/external/bsd/libnv/dist

SRCS+=	netbsd32_compat_09.c

.include <bsd.kmodule.mk>
