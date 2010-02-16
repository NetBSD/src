#	$NetBSD: Makefile,v 1.2 2010/02/16 20:42:45 pooka Exp $
#

.PATH:	${.CURDIR}/../../../../dev/usb ${.CURDIR}/../../../../kern

LIB=	rumpdev_ulpt

SRCS=	ulpt.c

SRCS+=	ulpt_at_usb.c

IOCONFIG= ULPT.ioconf

CPPFLAGS+=	-I${RUMPTOP}/librump/rumpvfs

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
