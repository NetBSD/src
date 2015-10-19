#	$NetBSD: Makefile,v 1.4 2015/10/19 16:16:32 pooka Exp $
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
