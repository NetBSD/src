#	$NetBSD: Makefile,v 1.5 2013/04/04 01:48:19 pooka Exp $
#

.PATH:	${.CURDIR}/../../../../dev/putter

LIB=	rumpdev_putter

SRCS=	putter.c
SRCS+=	component.c

CPPFLAGS+=	-I${RUMPTOP}/librump/rumpvfs

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
