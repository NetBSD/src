#	$NetBSD: Makefile,v 1.4.6.3 2016/03/19 11:30:34 skrll Exp $
#

.PATH:	${.CURDIR}/../../../../dev

LIB=	rumpdev_cgd
COMMENT=Cryptographic disk driver (block device crypto layer)

IOCONF=	CGD.ioconf
SRCS=   cgd.c cgd_crypto.c

SRCS+=	cgd_component.c

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
