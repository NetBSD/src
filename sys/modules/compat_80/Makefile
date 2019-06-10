#	$NetBSD: Makefile,v 1.2.4.2 2019/06/10 22:09:15 christos Exp $

.include "../Makefile.inc"

.PATH:	${S}/compat/common

KMOD=	compat_80

CPPFLAGS+=	-DCOMPAT_80

SRCS+=	compat_80_mod.c
SRCS+=	kern_mod_80.c

.include <bsd.kmodule.mk>
