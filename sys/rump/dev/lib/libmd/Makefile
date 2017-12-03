#	$NetBSD: Makefile,v 1.1.20.2 2017/12/03 11:39:07 jdolecek Exp $
#

.PATH:	${.CURDIR}/../../../../dev

LIB=	rumpdev_md
COMMENT=Memory disk device driver

IOCONF=	MD.ioconf

SRCS=	md.c
SRCS+=	md_component.c

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
