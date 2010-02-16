#	$NetBSD: Makefile,v 1.3 2010/02/16 20:42:44 pooka Exp $
#

.PATH:	${.CURDIR}/../../../../dev

LIB=	rumpdev_cgd

SRCS=   cgd.c cgd_crypto.c

SRCS+=	component.c

CPPFLAGS+=	-I${RUMPTOP}/librump/rumpvfs

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
