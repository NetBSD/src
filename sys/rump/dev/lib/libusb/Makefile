#	$NetBSD: Makefile,v 1.3 2010/02/16 20:42:45 pooka Exp $
#

.PATH:	${.CURDIR}/../../../../dev/usb

LIB=	rumpdev_usb

.if ${MACHINE} == "i386"
SRCS=	usb.c usbdi.c usbdi_util.c usb_mem.c usb_subr.c usb_quirks.c	\
	uhub.c usbroothub_subr.c
.else
SRCS=	dummy.c
.endif

#CPPFLAGS+= -DUHUB_DEBUG

CPPFLAGS+=	-I${.CURDIR}/opt

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
