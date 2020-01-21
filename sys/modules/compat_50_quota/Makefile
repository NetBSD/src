#	$NetBSD: Makefile,v 1.1 2020/01/21 02:37:17 pgoyette Exp $

.include "../Makefile.inc"

.PATH:	${S}/compat/common

KMOD=	compat_50_quota

CPPFLAGS+=	-DCOMPAT_50 -DCOMPAT_60 -DCOMPAT_70 -DCOMPAT_80
CPPFLAGS+=	-DQUOTA

SRCS+=	compat_50_quota.c

.include <bsd.kmodule.mk>
