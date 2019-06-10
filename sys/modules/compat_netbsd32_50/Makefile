#	$NetBSD: Makefile,v 1.3.4.2 2019/06/10 22:09:17 christos Exp $

.include "../Makefile.inc"

.PATH:	${S}/compat/netbsd32

KMOD=	compat_netbsd32_50

CPPFLAGS+=	-DNTP
CPPFLAGS+=	-DCOMPAT_NETBSD32
CPPFLAGS+=	-DCOMPAT_50 -DCOMPAT_60 -DCOMPAT_70 -DCOMPAT_80
CPPFLAGS+=	-I${NETBSDSRCDIR}/sys/external/bsd/libnv/dist

SRCS+=	netbsd32_compat_50.c netbsd32_rndpseudo_50.c

WARNS=	3

.include <bsd.kmodule.mk>
