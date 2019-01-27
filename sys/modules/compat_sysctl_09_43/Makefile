#	$NetBSD: Makefile,v 1.2 2019/01/27 02:08:47 pgoyette Exp $

.include "../Makefile.inc"

.PATH:	${S}/compat/common

KMOD=	compat_sysctl_09_43

CPPFLAGS+=	-DCOMPAT_09 -DCOMPAT_43

SRCS+=	compat_sysctl_09_43.c

.include <bsd.kmodule.mk>
