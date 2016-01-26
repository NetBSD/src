#	$NetBSD: Makefile,v 1.7 2016/01/26 23:12:15 pooka Exp $
#

.PATH:	${.CURDIR}/../../../../dev

LIB=	rumpdev_cgd
COMMENT=Cryptographic disk driver (block device crypto layer)

IOCONF=	CGD.ioconf
SRCS=   cgd.c cgd_crypto.c

SRCS+=	cgd_component.c

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
