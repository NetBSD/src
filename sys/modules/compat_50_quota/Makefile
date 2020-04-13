#	$NetBSD: Makefile,v 1.1.6.2 2020/04/13 08:05:06 martin Exp $

.include "../Makefile.inc"

.PATH:	${S}/compat/common

KMOD=	compat_50_quota

CPPFLAGS+=	-DCOMPAT_50 -DCOMPAT_60 -DCOMPAT_70 -DCOMPAT_80
CPPFLAGS+=	-DQUOTA

SRCS+=	compat_50_quota.c

.include <bsd.kmodule.mk>
