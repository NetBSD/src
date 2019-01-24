#	$NetBSD: Makefile,v 1.1.2.3 2019/01/24 02:06:35 pgoyette Exp $

.include "../Makefile.inc"

KMOD=		compat_raid_80
CPPFLAGS+=	-DCOMPAT_80

.PATH:	${S}/dev/raidframe

SRCS+=	rf_compat80.c 

.include <bsd.kmodule.mk>
