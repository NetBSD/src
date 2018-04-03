#	$NetBSD: Makefile,v 1.1.2.3 2018/04/03 08:29:44 pgoyette Exp $

.include "../Makefile.inc"

.PATH:	${S}/compat/common

KMOD=	compat_80

CPPFLAGS+=	-DCOMPAT_80

SRCS+=	compat_80_mod.c
SRCS+=	kern_mod_80.c

.PATH:	${S}/dev/raidframe

SRCS+=	rf_compat80.c 

.include <bsd.kmodule.mk>
