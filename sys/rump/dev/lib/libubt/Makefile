#	$NetBSD: Makefile,v 1.3.6.2 2010/08/11 22:55:01 yamt Exp $
#

.PATH:	${.CURDIR}/../../../../dev/usb ${.CURDIR}/../../../../dev/bluetooth

LIB=	rumpdev_ubt
IOCONF= UBT.ioconf

SRCS=	ubt.c bthub.c

SRCS+=	ubt_at_usb.c

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
