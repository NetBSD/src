#	$NetBSD: Makefile,v 1.2.4.2 2019/06/10 22:09:19 christos Exp $

.include "../Makefile.inc"

KMOD=		compat_raid_80
CPPFLAGS+=	-DCOMPAT_80

.PATH:	${S}/dev/raidframe

SRCS+=	rf_compat80.c 

.include <bsd.kmodule.mk>
