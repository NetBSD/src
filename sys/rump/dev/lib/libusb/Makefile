#	$NetBSD: Makefile,v 1.4.6.2 2010/04/30 14:44:25 uebayasi Exp $
#

.PATH:	${.CURDIR}/../../../../dev/usb

LIB=	rumpdev_usb

.if ${MACHINE} == "i386"
IOCONF= USB.ioconf

SRCS=	usb.c usbdi.c usbdi_util.c usb_mem.c usb_subr.c usb_quirks.c	\
	uhub.c usbroothub_subr.c
SRCS+=	usb_at_ugenhc.c
.else
SRCS=	dummy.c
.endif

#CPPFLAGS+= -DUHUB_DEBUG

CPPFLAGS+=	-I${.CURDIR}/opt

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
