#	$NetBSD: Makefile,v 1.1 2010/04/12 22:31:48 pooka Exp $
#

.PATH:	${.CURDIR}/../../../../dev

LIB=	rumpdev_fss

SRCS=   fss.c

SRCS+=	component.c

CPPFLAGS+=	-I${RUMPTOP}/librump/rumpvfs

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
