#	$NetBSD: Makefile,v 1.1.2.1 2018/04/17 08:02:35 pgoyette Exp $

.include "../Makefile.inc"

.PATH:	${S}/compat/common

KMOD=	compat_sysctl_09_43

CPPFLAGS+=	-DCOMPAT_09 -DCOMPAT_43

SRCS+=	compat_sysctl_09_43.c

.include <bsd.kmodule.mk>
