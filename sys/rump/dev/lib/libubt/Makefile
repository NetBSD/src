#	$NetBSD: Makefile,v 1.4 2015/10/19 16:16:34 pooka Exp $
#

.PATH:	${.CURDIR}/../../../../dev/usb ${.CURDIR}/../../../../dev/bluetooth

LIB=	rumpdev_ubt
COMMENT=USB BlueTooth driver

IOCONF= UBT.ioconf

SRCS=	ubt.c bthub.c

SRCS+=	ubt_at_usb.c

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
