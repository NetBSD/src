#	$NetBSD: Makefile,v 1.2.6.2 2014/05/22 11:41:14 yamt Exp $
#

.PATH:	${.CURDIR}/../../../../kern

LIB=	rumpkern_time

SRCS+=	time_component.c

CPPFLAGS+= -I${RUMPTOP}/librump/rumpkern

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
