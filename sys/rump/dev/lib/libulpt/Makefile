#	$NetBSD: Makefile,v 1.3.42.2 2016/03/19 11:30:35 skrll Exp $
#

.PATH:	${.CURDIR}/../../../../dev/usb

LIB=	rumpdev_ulpt
COMMENT=USB printer driver

IOCONF=	ULPT.ioconf

SRCS=	ulpt.c

SRCS+=	ulpt_at_usb.c

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
