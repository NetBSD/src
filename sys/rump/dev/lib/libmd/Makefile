#	$NetBSD: Makefile,v 1.1.24.1 2014/05/18 17:46:15 rmind Exp $
#

.PATH:	${.CURDIR}/../../../../dev

LIB=	rumpdev_md
IOCONF=	MD.ioconf

SRCS=	md.c
SRCS+=	md_component.c

CPPFLAGS+=	-I${.CURDIR}/../../../librump/rumpvfs

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
