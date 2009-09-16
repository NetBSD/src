#	$NetBSD: Makefile,v 1.2.4.4 2009/09/16 13:38:06 yamt Exp $
#

LIB=	rumpnet_sockin

SRCS=	sockin.c
SRCS+=	component.c

CPPFLAGS+= -I${RUMPTOP}/librump/rumpkern

RUMP_ISDOMAIN=

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
