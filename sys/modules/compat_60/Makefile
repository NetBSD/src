#	$NetBSD: Makefile,v 1.1.2.5 2018/03/17 06:49:57 pgoyette Exp $

.include "../Makefile.inc"

.PATH:	${S}/compat/common

KMOD=	compat_60

CPPFLAGS+=	-DCOMPAT_60 -DCOMPAT_70 -DCOMPAT_80

SRCS+=	compat_60_mod.c kern_sa_60.c tty_60.c kern_time_60.c

.include <bsd.kmodule.mk>
