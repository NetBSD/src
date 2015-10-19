#	$NetBSD: Makefile,v 1.3 2015/10/19 16:16:35 pooka Exp $
#

.PATH:	${.CURDIR}/../../../../dev

LIB=	rumpdev_vnd
COMMENT=Present a regular file as a block device (/dev/vnd)

IOCONF=	VND.ioconf
SRCS=   vnd.c

SRCS+=	vnd_component.c

CPPFLAGS+=	-I${RUMPTOP}/librump/rumpvfs

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
