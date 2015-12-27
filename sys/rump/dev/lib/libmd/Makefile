#	$NetBSD: Makefile,v 1.2.6.1 2015/12/27 12:10:09 skrll Exp $
#

.PATH:	${.CURDIR}/../../../../dev

LIB=	rumpdev_md
COMMENT=Memory disk device driver

IOCONF=	MD.ioconf

SRCS=	md.c
SRCS+=	md_component.c

CPPFLAGS+=	-I${.CURDIR}/../../../librump/rumpvfs

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
