#	$NetBSD: Makefile,v 1.5.22.3 2017/12/03 11:39:19 jdolecek Exp $
#

LIB=	rumpnet_sockin
COMMENT=PF_INET/PF_INET6 via hypercalls

SRCS=	sockin.c
SRCS+=	sockin_component.c

RUMPCOMP_USER_SRCS=	sockin_user.c

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
