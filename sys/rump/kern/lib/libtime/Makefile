#	$NetBSD: Makefile,v 1.2.10.2 2014/08/20 00:04:40 tls Exp $
#

.PATH:	${.CURDIR}/../../../../kern

LIB=	rumpkern_time

SRCS+=	time_component.c

CPPFLAGS+= -I${RUMPTOP}/librump/rumpkern

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
