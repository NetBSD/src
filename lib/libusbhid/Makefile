#	$NetBSD: Makefile,v 1.1 2001/12/28 17:45:26 augustss Exp $

LIB=	usbhid
MAN=	usbhid.3

MLINKS=	usbhid.3 libusbhid.3 usbhid.3 hid_get_report_desc.3 \
	usbhid.3 hid_dispose_report_desc.3 \
	usbhid.3 hid_start_parse.3 usbhid.3 hid_end_parse.3 \
	usbhid.3 hid_get_item.3 usbhid.3 hid_report_size.3 \
	usbhid.3 hid_locate.3 usbhid.3 hid_usage_page.3 \
	usbhid.3 hid_usage_in_page.3 usbhid.3 hid_init.3 \
	usbhid.3 hid_get_data.3 usbhid.3 hid_set_data.3

SRCS=	descr.c parse.c usage.c data.c

INCS=	usbhid.h
INCSDIR=/usr/include

.include <bsd.own.mk>

.if ${MKSHARE} != "no"
FILES=  usb_hid_usages
FILESDIR=/usr/share/misc
.endif

.include <bsd.lib.mk>

