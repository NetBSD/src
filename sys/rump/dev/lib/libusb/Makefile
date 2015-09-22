#	$NetBSD: Makefile,v 1.6.32.3 2015/09/22 12:06:14 skrll Exp $
#

.PATH:	${.CURDIR}/../../../../dev/usb

LIB=	rumpdev_usb

.if ${MACHINE} == "i386" || ${MACHINE} == "amd64"
IOCONF= USB.ioconf

SRCS=	usb.c usbdi.c usbdi_util.c usb_mem.c usb_subr.c usb_quirks.c	\
	uhub.c usbroothub.c usb_verbose.c
SRCS+=	usb_at_hc.c
.else
SRCS=	dummy.c
.endif

#CPPFLAGS+= -DUHUB_DEBUG

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
