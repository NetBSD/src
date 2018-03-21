#	$NetBSD: Makefile,v 1.1.2.1 2018/03/21 10:12:49 pgoyette Exp $

.include "../Makefile.inc"

.PATH:	${S}/compat/common

KMOD=	compat_80

CPPFLAGS+=	-DCOMPAT_80

SRCS+=	compat_80_mod.c raidframe80.c 

.include <bsd.kmodule.mk>
