#	$NetBSD: Makefile,v 1.5.22.2 2014/08/20 00:04:43 tls Exp $
#

LIB=	rumpnet_sockin

SRCS=	sockin.c
SRCS+=	sockin_component.c

CPPFLAGS+= -I${RUMPTOP}/librump/rumpkern

RUMPCOMP_USER_SRCS=	sockin_user.c

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
