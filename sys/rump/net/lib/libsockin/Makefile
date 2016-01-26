#	$NetBSD: Makefile,v 1.10 2016/01/26 23:12:19 pooka Exp $
#

LIB=	rumpnet_sockin
COMMENT=PF_INET/PF_INET6 via hypercalls

SRCS=	sockin.c
SRCS+=	sockin_component.c

RUMPCOMP_USER_SRCS=	sockin_user.c

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
