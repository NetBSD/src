#	$NetBSD: Makefile,v 1.6 2011/01/31 00:10:06 christos Exp $
#

.PATH:	${.CURDIR}/../../../../dev/usb

LIB=	rumpdev_usb

.if ${MACHINE} == "i386" || ${MACHINE} == "amd64"
IOCONF= USB.ioconf

SRCS=	usb.c usbdi.c usbdi_util.c usb_mem.c usb_subr.c usb_quirks.c	\
	uhub.c usbroothub_subr.c usb_verbose.c
SRCS+=	usb_at_ugenhc.c
.else
SRCS=	dummy.c
.endif

#CPPFLAGS+= -DUHUB_DEBUG

CPPFLAGS+=	-I${.CURDIR}/opt

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
