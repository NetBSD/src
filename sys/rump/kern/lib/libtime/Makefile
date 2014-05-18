#	$NetBSD: Makefile,v 1.2.4.2 2014/05/18 17:46:18 rmind Exp $
#

.PATH:	${.CURDIR}/../../../../kern

LIB=	rumpkern_time

SRCS+=	time_component.c

CPPFLAGS+= -I${RUMPTOP}/librump/rumpkern

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
