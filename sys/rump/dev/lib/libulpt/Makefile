#	$NetBSD: Makefile,v 1.3.2.2 2010/03/11 15:04:34 yamt Exp $
#

.PATH:	${.CURDIR}/../../../../dev/usb

LIB=	rumpdev_ulpt
IOCONF=	ULPT.ioconf

SRCS=	ulpt.c

SRCS+=	ulpt_at_usb.c

CPPFLAGS+=	-I${RUMPTOP}/librump/rumpvfs

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
