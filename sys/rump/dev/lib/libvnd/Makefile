#	$NetBSD: Makefile,v 1.1.6.3 2017/12/03 11:39:10 jdolecek Exp $
#

.PATH:	${.CURDIR}/../../../../dev

LIB=	rumpdev_vnd
COMMENT=Present a regular file as a block device (/dev/vnd)

IOCONF=	VND.ioconf
SRCS=   vnd.c

SRCS+=	vnd_component.c

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
