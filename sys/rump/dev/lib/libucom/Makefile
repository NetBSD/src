#	$NetBSD: Makefile,v 1.5.36.2 2016/03/19 11:30:35 skrll Exp $
#

.PATH:	${.CURDIR}/../../../../dev/usb ${.CURDIR}/../../../../kern

LIB=	rumpdev_ucom
COMMENT=USB serial driver

IOCONF= UCOM.ioconf

SRCS=	ucom.c uplcom.c u3g.c

SRCS+=	ucom_at_usb.c

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
