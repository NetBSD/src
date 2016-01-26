#	$NetBSD: Makefile,v 1.4 2016/01/26 23:12:15 pooka Exp $
#

.PATH:	${.CURDIR}/../../../../dev

LIB=	rumpdev_md
COMMENT=Memory disk device driver

IOCONF=	MD.ioconf

SRCS=	md.c
SRCS+=	md_component.c

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
