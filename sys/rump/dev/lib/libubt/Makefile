#	$NetBSD: Makefile,v 1.5 2015/11/11 21:52:46 pooka Exp $
#

.PATH:	${.CURDIR}/../../../../dev/usb ${.CURDIR}/../../../../dev/bluetooth

LIB=	rumpdev_ubt
COMMENT=USB BlueTooth driver

IOCONF= UBT.ioconf
COMPONENT_SIMPLE= ubt

SRCS=	ubt.c bthub.c

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
