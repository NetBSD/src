#	$NetBSD: Makefile,v 1.3.42.1 2015/12/27 12:10:11 skrll Exp $
#

.PATH:	${.CURDIR}/../../../../dev/usb ${.CURDIR}/../../../../dev/bluetooth

LIB=	rumpdev_ubt
COMMENT=USB BlueTooth driver

IOCONF= UBT.ioconf
RUMP_COMPONENT=ioconf

SRCS=	ubt.c bthub.c

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
