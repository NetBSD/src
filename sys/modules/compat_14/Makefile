#	$NetBSD: Makefile,v 1.1.2.1 2018/03/30 10:09:08 pgoyette Exp $

.include "../Makefile.inc"

.PATH:	${S}/compat/common

KMOD=	compat_14

CPPFLAGS+=	-DCOMPAT_14 -DCOMPAT_16 -DCOMPAT_20 -DCOMPAT_30
CPPFLAGS+=	-DCOMPAT_40 -DCOMPAT_50 -DCOMPAT_60 -DCOMPAT_70 -DCOMPAT_80

SRCS+=	compat_14_mod.c
SRCS+=	rtsock_14.c


.include <bsd.kmodule.mk>
