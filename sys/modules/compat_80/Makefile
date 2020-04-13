#	$NetBSD: Makefile,v 1.2.4.3 2020/04/13 08:05:06 martin Exp $

.include "../Makefile.inc"

.PATH:	${S}/compat/common

KMOD=	compat_80

CPPFLAGS+=	-DCOMPAT_80

SRCS+=	compat_80_mod.c
SRCS+=	kern_mod_80.c
SRCS+=	if_media_80.c

.include <bsd.kmodule.mk>
