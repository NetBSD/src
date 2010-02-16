#	$NetBSD: Makefile,v 1.2 2010/02/16 20:42:45 pooka Exp $
#

.PATH:	${.CURDIR}/../../../../dev/usb

LIB=	rumpdev_usbrum

SRCS=   if_rum.c

SRCS+=	rum_at_usb.c

CPPFLAGS+=	-I${.CURDIR}/opt -I${RUMPTOP}/librump/rumpnet/opt

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
