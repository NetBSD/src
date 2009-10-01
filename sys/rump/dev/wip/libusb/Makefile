#	$NetBSD: Makefile,v 1.1 2009/10/01 21:46:31 pooka Exp $
#

.PATH:	${.CURDIR}/../../../../dev/usb

LIB=	rumpdev_usb

# sys/kern
SRCS=	usb.c usbdi.c usbdi_util.c usb_mem.c usb_subr.c usb_quirks.c uhub.c

# XXX: doesn't belong here by a longshot, but it's the easy choice for now
SRCS+=	bus_dma.c

CFLAGS+=	-Wno-pointer-sign
CPPFLAGS+=	-I${.CURDIR}/opt

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
