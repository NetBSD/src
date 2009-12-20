#	$NetBSD: Makefile,v 1.1 2009/12/20 15:43:13 pooka Exp $
#

.PATH:	${.CURDIR}/../../../../dev/usb

LIB=	rumpdev_ucom

SRCS=	ucom.c uplcom.c

SRCS+=	ucom_at_usb.c

CFLAGS+=	-Wno-pointer-sign
CPPFLAGS+=	-I${RUMPTOP}/librump/rumpvfs

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
