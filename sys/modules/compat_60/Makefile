#	$NetBSD: Makefile,v 1.1.2.1 2018/03/15 23:23:36 pgoyette Exp $

.include "../Makefile.inc"

.PATH:	${S}/compat/common

KMOD=	compat_60

CPPFLAGS+=	-D COMPAT_60 -DCOMPAT_70 -DCOMPAT_80

SRCS+=	kern_sa_60.c tty_60.c kern_time_60.c

.include <bsd.kmodule.mk>
