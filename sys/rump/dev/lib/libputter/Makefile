#	$NetBSD: Makefile,v 1.3.14.1 2013/06/23 06:20:27 tls Exp $
#

.PATH:	${.CURDIR}/../../../../dev/putter

LIB=	rumpdev_putter

SRCS=	putter.c
SRCS+=	component.c

CPPFLAGS+=	-I${RUMPTOP}/librump/rumpvfs

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
