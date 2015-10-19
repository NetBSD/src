#	$NetBSD: Makefile,v 1.6 2015/10/19 16:16:32 pooka Exp $
#

.PATH:	${.CURDIR}/../../../../dev

LIB=	rumpdev_cgd
COMMENT=Cryptographic disk driver (block device crypto layer)

IOCONF=	CGD.ioconf
SRCS=   cgd.c cgd_crypto.c

SRCS+=	cgd_component.c

CPPFLAGS+=	-I${RUMPTOP}/librump/rumpvfs

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
