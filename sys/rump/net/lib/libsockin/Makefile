#	$NetBSD: Makefile,v 1.9 2015/10/19 16:16:39 pooka Exp $
#

LIB=	rumpnet_sockin
COMMENT=PF_INET/PF_INET6 via hypercalls

SRCS=	sockin.c
SRCS+=	sockin_component.c

CPPFLAGS+= -I${RUMPTOP}/librump/rumpkern

RUMPCOMP_USER_SRCS=	sockin_user.c

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
