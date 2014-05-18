#	$NetBSD: Makefile,v 1.1.28.1 2014/05/18 17:46:15 rmind Exp $
#

.PATH:	${.CURDIR}/../../../../dev

LIB=	rumpdev_fss

SRCS=   fss.c

SRCS+=	fss_component.c

CPPFLAGS+=	-I${RUMPTOP}/librump/rumpvfs

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
