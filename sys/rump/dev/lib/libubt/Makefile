#	$NetBSD: Makefile,v 1.2 2010/03/22 14:47:02 pooka Exp $
#

.PATH:	${.CURDIR}/../../../../dev/usb ${.CURDIR}/../../../../dev/bluetooth

LIB=	rumpdev_ubt
IOCONF= UBT.ioconf

SRCS=	ubt.c bthub.c

SRCS+=	ubt_at_usb.c

CPPFLAGS+= -I${RUMPTOP}/net/lib/libnetbt

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
