#	$NetBSD: Makefile,v 1.1.2.4 2018/10/03 22:50:50 pgoyette Exp $

.include "../Makefile.inc"

.PATH:	${S}/compat/common

KMOD=	compat_80

CPPFLAGS+=	-DCOMPAT_80

SRCS+=	compat_80_mod.c
SRCS+=	kern_mod_80.c

.include <bsd.kmodule.mk>
