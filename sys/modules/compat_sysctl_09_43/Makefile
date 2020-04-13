#	$NetBSD: Makefile,v 1.3.4.3 2020/04/13 08:05:10 martin Exp $

.include "../Makefile.inc"

.PATH:	${S}/compat/common

KMOD=	compat_sysctl_09_43

CPPFLAGS+=	-DCOMPAT_09 -DCOMPAT_43

SRCS+=	compat_sysctl_09_43.c

.include <bsd.kmodule.mk>
