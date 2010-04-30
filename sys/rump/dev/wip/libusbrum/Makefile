#	$NetBSD: Makefile,v 1.1.2.1 2010/04/30 14:44:27 uebayasi Exp $
#

.PATH:	${.CURDIR}/../../../../dev/usb

LIB=	rumpdev_usbrum

SRCS=   if_rum.c

SRCS+=	rum_at_usb.c

CPPFLAGS+=	-I${.CURDIR}/opt -I${RUMPTOP}/librump/rumpnet/opt

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
