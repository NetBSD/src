#	$NetBSD: Makefile,v 1.3.4.2 2019/06/10 22:09:19 christos Exp $

.include "../Makefile.inc"

.PATH:	${S}/compat/common

KMOD=	compat_sysctl_09_43

CPPFLAGS+=	-DCOMPAT_09 -DCOMPAT_43

SRCS+=	compat_sysctl_09_43.c

WARNS=	3

.include <bsd.kmodule.mk>
