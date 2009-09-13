#	$NetBSD: Makefile,v 1.4 2009/09/13 22:51:42 pooka Exp $
#

LIB=	rumpnet_sockin

SRCS=	sockin.c
SRCS+=	component.c

CPPFLAGS+= -I${RUMPTOP}/librump/rumpkern

RUMP_ISDOMAIN=

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
