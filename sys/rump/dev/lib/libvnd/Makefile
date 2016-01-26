#	$NetBSD: Makefile,v 1.4 2016/01/26 23:12:16 pooka Exp $
#

.PATH:	${.CURDIR}/../../../../dev

LIB=	rumpdev_vnd
COMMENT=Present a regular file as a block device (/dev/vnd)

IOCONF=	VND.ioconf
SRCS=   vnd.c

SRCS+=	vnd_component.c

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
