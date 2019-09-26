#	$NetBSD: Makefile,v 1.4 2019/09/26 02:01:03 christos Exp $

.include "../Makefile.inc"

.PATH:	${S}/compat/netbsd32

KMOD=	compat_netbsd32_80

CPPFLAGS+=	-DCOMPAT_NETBSD32
CPPFLAGS+=	-DCOMPAT_80
CPPFLAGS+=	-I${NETBSDSRCDIR}/sys/external/bsd/libnv/dist

SRCS+=	netbsd32_compat_80.c

.include <bsd.kmodule.mk>
