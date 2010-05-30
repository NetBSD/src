#	$NetBSD: Makefile,v 1.3.4.2 2010/05/30 05:18:04 rmind Exp $
#

.PATH:	${.CURDIR}/../../../../dev/usb ${.CURDIR}/../../../../dev/bluetooth

LIB=	rumpdev_ubt
IOCONF= UBT.ioconf

SRCS=	ubt.c bthub.c

SRCS+=	ubt_at_usb.c

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
