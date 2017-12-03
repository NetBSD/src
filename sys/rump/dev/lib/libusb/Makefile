#	$NetBSD: Makefile,v 1.6.14.1 2017/12/03 11:39:10 jdolecek Exp $
#

.PATH:	${.CURDIR}/../../../../dev/usb

LIB=	rumpdev_usb
COMMENT=USB support

.if ${MACHINE} == "i386" || ${MACHINE} == "amd64"
IOCONF= USB.ioconf
RUMP_COMPONENT=ioconf

SRCS=	usb.c usbdi.c usbdi_util.c usb_mem.c usb_subr.c usb_quirks.c	\
	uhub.c usbroothub.c usb_verbose.c
.else
SRCS=	dummy.c
.endif

#CPPFLAGS+= -DUHUB_DEBUG

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
