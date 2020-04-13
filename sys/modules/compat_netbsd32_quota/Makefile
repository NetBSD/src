#	$NetBSD: Makefile,v 1.1.4.2 2020/04/13 08:05:09 martin Exp $

.include "../Makefile.inc"
.include "../Makefile.assym"

KMOD=	compat_netbsd32_quota

CPPFLAGS+=	-DCOMPAT_NETBSD32
CPPFLAGS+=	-DQUOTA

.PATH:	${S}/compat/netbsd32

SRCS+=	netbsd32_quota.c

.include <bsd.kmodule.mk>
