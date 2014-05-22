#	$NetBSD: Makefile,v 1.1.14.1 2014/05/22 11:41:12 yamt Exp $
#

.PATH:	${.CURDIR}/../../../../dev

LIB=	rumpdev_fss

SRCS=   fss.c

SRCS+=	fss_component.c

CPPFLAGS+=	-I${RUMPTOP}/librump/rumpvfs

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
