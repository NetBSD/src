#	$NetBSD: Makefile,v 1.1 2014/07/17 08:16:12 prlw1 Exp $
#

.PATH:	${.CURDIR}/../../../../dev

LIB=	rumpdev_vnd

SRCS=   vnd.c

SRCS+=	vnd_component.c

CPPFLAGS+=	-I${RUMPTOP}/librump/rumpvfs

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
