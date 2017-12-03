#	$NetBSD: Makefile,v 1.3.24.1 2017/12/03 11:39:10 jdolecek Exp $
#

.PATH:	${.CURDIR}/../../../../dev/usb

LIB=	rumpdev_ulpt
COMMENT=USB printer driver

IOCONF=	ULPT.ioconf

SRCS=	ulpt.c

SRCS+=	ulpt_at_usb.c

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
