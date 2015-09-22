#	$NetBSD: Makefile,v 1.2.6.1 2015/09/22 12:06:12 skrll Exp $
#

.PATH:	${.CURDIR}/../../../../dev

LIB=	rumpdev_fss
IOCONF=	FSS.ioconf
SRCS=   fss.c

SRCS+=	fss_component.c

CPPFLAGS+=	-I${RUMPTOP}/librump/rumpvfs

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
