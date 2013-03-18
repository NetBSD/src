#	$NetBSD: Makefile,v 1.6 2013/03/18 13:14:11 pooka Exp $
#

LIB=	rumpnet_sockin

SRCS=	sockin.c
SRCS+=	component.c

CPPFLAGS+= -I${RUMPTOP}/librump/rumpkern

RUMPCOMP_USER=	#defined

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
