#	$NetBSD: Makefile,v 1.1.2.2 2018/10/04 01:55:17 pgoyette Exp $

.include "../Makefile.inc"

.PATH:	${S}/compat/common

KMOD=	compat_raid_80

CPPFLAGS+=	-DCOMPAT_80

.PATH:	${S}/dev/raidframe

SRCS+=	rf_compat80.c 

.include <bsd.kmodule.mk>
