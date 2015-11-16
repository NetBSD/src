#	$NetBSD: Makefile,v 1.6 2015/11/16 23:27:08 pooka Exp $
#

.PATH:	${.CURDIR}/../../../../dev/usb ${.CURDIR}/../../../../dev/bluetooth

LIB=	rumpdev_ubt
COMMENT=USB BlueTooth driver

IOCONF= UBT.ioconf
RUMP_COMPONENT=ioconf

SRCS=	ubt.c bthub.c

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
