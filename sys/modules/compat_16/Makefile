#	$NetBSD: Makefile,v 1.2 2019/01/27 02:08:44 pgoyette Exp $

.include "../Makefile.inc"

.PATH:	${S}/compat/common

KMOD=	compat_16

CPPFLAGS+=	-DCOMPAT_16 -DCOMPAT_20 -DCOMPAT_30
CPPFLAGS+=	-DCOMPAT_40 -DCOMPAT_50 -DCOMPAT_60 -DCOMPAT_70 -DCOMPAT_80

SRCS+=	compat_16_mod.c
SRCS+=	kern_sig_16.c

.include <bsd.kmodule.mk>
