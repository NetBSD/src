#	$NetBSD: Makefile,v 1.2.6.2 2015/12/27 12:10:09 skrll Exp $
#

.PATH:	${.CURDIR}/../../../../dev

LIB=	rumpdev_fss
COMMENT=File system snapshot device

IOCONF=	FSS.ioconf
SRCS=   fss.c

SRCS+=	fss_component.c

CPPFLAGS+=	-I${RUMPTOP}/librump/rumpvfs

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
