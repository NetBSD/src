#	$NetBSD: Makefile,v 1.1.2.2 2014/08/10 06:56:50 tls Exp $
#

.PATH:	${.CURDIR}/../../../../dev

LIB=	rumpdev_vnd

SRCS=   vnd.c

SRCS+=	vnd_component.c

CPPFLAGS+=	-I${RUMPTOP}/librump/rumpvfs

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
