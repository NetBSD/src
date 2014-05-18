#	$NetBSD: Makefile,v 1.6.6.1 2014/05/18 17:46:20 rmind Exp $
#

LIB=	rumpnet_sockin

SRCS=	sockin.c
SRCS+=	sockin_component.c

CPPFLAGS+= -I${RUMPTOP}/librump/rumpkern

RUMPCOMP_USER_SRCS=	sockin_user.c

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
