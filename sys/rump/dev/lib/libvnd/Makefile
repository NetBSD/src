#	$NetBSD: Makefile,v 1.1.6.2 2014/08/20 00:04:38 tls Exp $
#

.PATH:	${.CURDIR}/../../../../dev

LIB=	rumpdev_vnd

SRCS=   vnd.c

SRCS+=	vnd_component.c

CPPFLAGS+=	-I${RUMPTOP}/librump/rumpvfs

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
