#	$NetBSD: Makefile,v 1.3.24.1 2017/12/03 11:39:09 jdolecek Exp $
#

.PATH:	${.CURDIR}/../../../../dev/usb ${.CURDIR}/../../../../dev/bluetooth

LIB=	rumpdev_ubt
COMMENT=USB BlueTooth driver

IOCONF= UBT.ioconf
RUMP_COMPONENT=ioconf

SRCS=	ubt.c bthub.c

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
