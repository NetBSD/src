#	$NetBSD: Makefile,v 1.5.18.1 2017/12/03 11:39:09 jdolecek Exp $
#

.PATH:	${.CURDIR}/../../../../dev/usb ${.CURDIR}/../../../../kern

LIB=	rumpdev_ucom
COMMENT=USB serial driver

IOCONF= UCOM.ioconf

SRCS=	ucom.c uplcom.c u3g.c

SRCS+=	ucom_at_usb.c

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
