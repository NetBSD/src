#	$NetBSD: Makefile,v 1.7 2014/03/13 01:40:30 pooka Exp $
#

LIB=	rumpnet_sockin

SRCS=	sockin.c
SRCS+=	component.c

CPPFLAGS+= -I${RUMPTOP}/librump/rumpkern

RUMPCOMP_USER_SRCS=	sockin_user.c

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
