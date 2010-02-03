#	$NetBSD: Makefile,v 1.3 2010/02/03 21:18:38 pooka Exp $
#

.PATH:	${.CURDIR}/../../../../dev/usb

LIB=	rumpdev_usb

SRCS=	usb.c usbdi.c usbdi_util.c usb_mem.c usb_subr.c usb_quirks.c	\
	uhub.c usbroothub_subr.c

# XXX: doesn't belong here by a longshot, but it's the easy choice for now
SRCS+=	bus_dma.c

#CPPFLAGS+= -DUHUB_DEBUG

CFLAGS+=	-Wno-pointer-sign
CPPFLAGS+=	-I${.CURDIR}/opt

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
