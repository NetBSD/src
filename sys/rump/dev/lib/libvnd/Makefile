#	$NetBSD: Makefile,v 1.1.8.3 2016/03/19 11:30:35 skrll Exp $
#

.PATH:	${.CURDIR}/../../../../dev

LIB=	rumpdev_vnd
COMMENT=Present a regular file as a block device (/dev/vnd)

IOCONF=	VND.ioconf
SRCS=   vnd.c

SRCS+=	vnd_component.c

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
