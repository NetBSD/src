#	$NetBSD: Makefile,v 1.4 2010/02/09 18:57:03 pooka Exp $
#

.PATH:	${.CURDIR}/../../../../dev/usb

LIB=	rumpdev_usb

SRCS=	usb.c usbdi.c usbdi_util.c usb_mem.c usb_subr.c usb_quirks.c	\
	uhub.c usbroothub_subr.c

#CPPFLAGS+= -DUHUB_DEBUG

CFLAGS+=	-Wno-pointer-sign
CPPFLAGS+=	-I${.CURDIR}/opt

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
