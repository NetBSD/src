#	$NetBSD: Makefile,v 1.2 2014/03/13 01:59:05 pooka Exp $
#

.PATH:	${.CURDIR}/../../../../dev

LIB=	rumpdev_fss

SRCS=   fss.c

SRCS+=	fss_component.c

CPPFLAGS+=	-I${RUMPTOP}/librump/rumpvfs

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
