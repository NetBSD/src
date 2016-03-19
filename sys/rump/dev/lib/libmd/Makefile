#	$NetBSD: Makefile,v 1.2.6.2 2016/03/19 11:30:34 skrll Exp $
#

.PATH:	${.CURDIR}/../../../../dev

LIB=	rumpdev_md
COMMENT=Memory disk device driver

IOCONF=	MD.ioconf

SRCS=	md.c
SRCS+=	md_component.c

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
