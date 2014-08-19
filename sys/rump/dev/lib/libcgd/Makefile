#	$NetBSD: Makefile,v 1.3.20.1 2014/08/20 00:04:37 tls Exp $
#

.PATH:	${.CURDIR}/../../../../dev

LIB=	rumpdev_cgd

SRCS=   cgd.c cgd_crypto.c

SRCS+=	cgd_component.c

CPPFLAGS+=	-I${RUMPTOP}/librump/rumpvfs

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
