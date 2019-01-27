#	$NetBSD: Makefile,v 1.2 2019/01/27 02:08:47 pgoyette Exp $

.include "../Makefile.inc"

KMOD=		compat_raid_80
CPPFLAGS+=	-DCOMPAT_80

.PATH:	${S}/dev/raidframe

SRCS+=	rf_compat80.c 

.include <bsd.kmodule.mk>
