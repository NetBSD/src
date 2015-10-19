#	$NetBSD: Makefile,v 1.3 2015/10/19 16:16:33 pooka Exp $
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
