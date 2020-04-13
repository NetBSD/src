#	$NetBSD: Makefile,v 1.1.4.2 2020/04/13 08:05:09 martin Exp $

.include "../Makefile.inc"
.include "../Makefile.assym"

KMOD=	compat_netbsd32_quota_50

CPPFLAGS+=	-DCOMPAT_NETBSD32
CPPFLAGS+=	-DCOMPAT_50 -DQUOTA

.PATH:	${S}/compat/netbsd32

SRCS+=	netbsd32_compat_50_quota.c

.include <bsd.kmodule.mk>
