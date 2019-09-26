#	$NetBSD: Makefile,v 1.5 2019/09/26 02:01:02 christos Exp $

.include "../Makefile.inc"

.PATH:	${S}/compat/common

KMOD=	compat_80

CPPFLAGS+=	-DCOMPAT_80

SRCS+=	compat_80_mod.c
SRCS+=	kern_mod_80.c
SRCS+=	if_media_80.c

.include <bsd.kmodule.mk>
