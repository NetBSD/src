#	$NetBSD: Makefile,v 1.1.4.2 2010/05/30 05:18:04 rmind Exp $
#

.PATH:	${.CURDIR}/../../../../dev

LIB=	rumpdev_fss

SRCS=   fss.c

SRCS+=	component.c

CPPFLAGS+=	-I${RUMPTOP}/librump/rumpvfs

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
