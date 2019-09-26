#	$NetBSD: Makefile,v 1.4 2019/09/26 02:01:05 christos Exp $

.include "../Makefile.inc"

.PATH:	${S}/compat/common

KMOD=	compat_sysctl_09_43

CPPFLAGS+=	-DCOMPAT_09 -DCOMPAT_43

SRCS+=	compat_sysctl_09_43.c

.include <bsd.kmodule.mk>
