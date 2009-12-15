#	$NetBSD: Makefile,v 1.1 2009/12/15 15:52:30 pooka Exp $
#

.PATH:	${.CURDIR}/../../../../dev/usb ${.CURDIR}/../../../../kern

LIB=	rumpdev_ulpt

SRCS=	ulpt.c

SRCS+=	ulpt_at_usb.c

CFLAGS+=	-Wno-pointer-sign
CPPFLAGS+=	-I${RUMPTOP}/librump/rumpvfs

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
