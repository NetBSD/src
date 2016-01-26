#	$NetBSD: Makefile,v 1.5 2016/01/26 23:12:16 pooka Exp $
#

.PATH:	${.CURDIR}/../../../../dev/usb

LIB=	rumpdev_ulpt
COMMENT=USB printer driver

IOCONF=	ULPT.ioconf

SRCS=	ulpt.c

SRCS+=	ulpt_at_usb.c

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
