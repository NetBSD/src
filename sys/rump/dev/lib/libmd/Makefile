#	$NetBSD: Makefile,v 1.1.20.1 2014/08/20 00:04:37 tls Exp $
#

.PATH:	${.CURDIR}/../../../../dev

LIB=	rumpdev_md
IOCONF=	MD.ioconf

SRCS=	md.c
SRCS+=	md_component.c

CPPFLAGS+=	-I${.CURDIR}/../../../librump/rumpvfs

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
