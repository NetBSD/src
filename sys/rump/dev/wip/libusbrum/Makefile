#	$NetBSD: Makefile,v 1.1 2009/10/04 10:43:03 pooka Exp $
#

.PATH:	${.CURDIR}/../../../../dev/usb

LIB=	rumpdev_usbrum

SRCS=   if_rum.c

SRCS+=	rum_at_usb.c

CFLAGS+=	-Wno-pointer-sign
CPPFLAGS+=	-I${.CURDIR}/opt -I${RUMPTOP}/librump/rumpnet/opt

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
