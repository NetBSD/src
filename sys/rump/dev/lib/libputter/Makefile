#	$NetBSD: Makefile,v 1.3.14.2 2014/08/20 00:04:38 tls Exp $
#

.PATH:	${.CURDIR}/../../../../dev/putter

LIB=	rumpdev_putter

SRCS=	putter.c
SRCS+=	putter_component.c

CPPFLAGS+=	-I${RUMPTOP}/librump/rumpvfs

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
