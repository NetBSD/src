#	$NetBSD: Makefile,v 1.4.6.3 2010/08/17 06:47:56 uebayasi Exp $
#

.PATH:	${.CURDIR}/../../../../dev/usb ${.CURDIR}/../../../../kern

LIB=	rumpdev_ucom
IOCONF= UCOM.ioconf

SRCS=	ucom.c uplcom.c u3g.c

SRCS+=	ucom_at_usb.c

CPPFLAGS+=	-I${RUMPTOP}/librump/rumpvfs

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
