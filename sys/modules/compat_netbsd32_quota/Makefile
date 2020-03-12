#	$NetBSD: Makefile,v 1.1 2020/03/12 15:02:29 pgoyette Exp $

.include "../Makefile.inc"
.include "../Makefile.assym"

KMOD=	compat_netbsd32_quota

CPPFLAGS+=	-DCOMPAT_NETBSD32
CPPFLAGS+=	-DQUOTA

.PATH:	${S}/compat/netbsd32

SRCS+=	netbsd32_quota.c

.include <bsd.kmodule.mk>
