#	$NetBSD: Makefile,v 1.2.4.5 2010/03/11 15:04:40 yamt Exp $
#

LIB=	rumpnet_sockin

SRCS=	sockin.c
SRCS+=	component.c

CPPFLAGS+= -I${RUMPTOP}/librump/rumpkern

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
