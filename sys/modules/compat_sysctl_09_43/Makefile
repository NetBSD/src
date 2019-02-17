#	$NetBSD: Makefile,v 1.3 2019/02/17 04:05:46 rin Exp $

.include "../Makefile.inc"

.PATH:	${S}/compat/common

KMOD=	compat_sysctl_09_43

CPPFLAGS+=	-DCOMPAT_09 -DCOMPAT_43

SRCS+=	compat_sysctl_09_43.c

WARNS=	3

.include <bsd.kmodule.mk>
