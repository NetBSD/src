#	$NetBSD: Makefile,v 1.3.20.2 2017/12/03 11:39:06 jdolecek Exp $
#

.PATH:	${.CURDIR}/../../../../dev

LIB=	rumpdev_cgd
COMMENT=Cryptographic disk driver (block device crypto layer)

IOCONF=	CGD.ioconf
SRCS=   cgd.c cgd_crypto.c

SRCS+=	cgd_component.c

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
