#	$NetBSD: Makefile,v 1.4 2014/03/13 01:50:51 pooka Exp $
#

.PATH:	${.CURDIR}/../../../../dev

LIB=	rumpdev_cgd

SRCS=   cgd.c cgd_crypto.c

SRCS+=	cgd_component.c

CPPFLAGS+=	-I${RUMPTOP}/librump/rumpvfs

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
