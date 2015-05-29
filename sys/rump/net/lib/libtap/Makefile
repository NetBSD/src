#	$NetBSD: Makefile,v 1.1 2015/05/29 12:32:23 pooka Exp $
#

.PATH:	${.CURDIR}/../../../../net

LIB=	rumpnet_tap

SRCS=	if_tap.c

SRCS+=	tap_component.c

CPPFLAGS+=	-I${.CURDIR}/../libnet/opt -I${.CURDIR}/../../../librump/rumpvfs

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
