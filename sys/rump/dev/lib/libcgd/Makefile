#	$NetBSD: Makefile,v 1.4.6.1 2015/09/22 12:06:12 skrll Exp $
#

.PATH:	${.CURDIR}/../../../../dev

LIB=	rumpdev_cgd
IOCONF=	CGD.ioconf
SRCS=   cgd.c cgd_crypto.c

SRCS+=	cgd_component.c

CPPFLAGS+=	-I${RUMPTOP}/librump/rumpvfs

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
