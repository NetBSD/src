#	$NetBSD: Makefile,v 1.1 2010/02/11 02:22:09 pooka Exp $
#

.PATH:	${.CURDIR}/../../../../dev/usb ${.CURDIR}/../../../../kern

LIB=	rumpdev_ulpt

SRCS=	ulpt.c

SRCS+=	ulpt_at_usb.c

IOCONFIG= ULPT.ioconf

CFLAGS+=	-Wno-pointer-sign
CPPFLAGS+=	-I${RUMPTOP}/librump/rumpvfs

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
