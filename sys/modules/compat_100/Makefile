#	$NetBSD: Makefile,v 1.1 2022/12/19 23:19:51 pgoyette Exp $

.include "../Makefile.inc"

.PATH:	${S}/compat/common

KMOD=	compat_100

CPPFLAGS+=	-DCOMPAT_100

SRCS+=	compat_100_mod.c

.include <bsd.kmodule.mk>
