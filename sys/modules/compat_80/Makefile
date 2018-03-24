#	$NetBSD: Makefile,v 1.1.2.2 2018/03/24 01:59:16 pgoyette Exp $

.include "../Makefile.inc"

.PATH:	${S}/compat/common

KMOD=	compat_80

CPPFLAGS+=	-DCOMPAT_80

SRCS+=	compat_80_mod.c

.PATH:	${S}/dev/raidframe

SRCS+=	rf_compat80.c 

.include <bsd.kmodule.mk>
