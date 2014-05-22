#	$NetBSD: Makefile,v 1.1.10.1 2014/05/22 11:41:12 yamt Exp $
#

.PATH:	${.CURDIR}/../../../../dev

LIB=	rumpdev_md
IOCONF=	MD.ioconf

SRCS=	md.c
SRCS+=	md_component.c

CPPFLAGS+=	-I${.CURDIR}/../../../librump/rumpvfs

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
