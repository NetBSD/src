#	$NetBSD: Makefile,v 1.2 2014/04/02 19:37:17 pooka Exp $
#

.PATH:	${.CURDIR}/../../../../kern

LIB=	rumpkern_time

SRCS+=	time_component.c

CPPFLAGS+= -I${RUMPTOP}/librump/rumpkern

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
