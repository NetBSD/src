#	$NetBSD: Makefile,v 1.8.6.2 2016/03/19 11:30:38 skrll Exp $
#

LIB=	rumpnet_sockin
COMMENT=PF_INET/PF_INET6 via hypercalls

SRCS=	sockin.c
SRCS+=	sockin_component.c

RUMPCOMP_USER_SRCS=	sockin_user.c

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
