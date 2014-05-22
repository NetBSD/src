#	$NetBSD: Makefile,v 1.5.12.1 2014/05/22 11:41:17 yamt Exp $
#

LIB=	rumpnet_sockin

SRCS=	sockin.c
SRCS+=	sockin_component.c

CPPFLAGS+= -I${RUMPTOP}/librump/rumpkern

RUMPCOMP_USER_SRCS=	sockin_user.c

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
