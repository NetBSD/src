#	$NetBSD: Makefile,v 1.4 1999/07/23 09:40:44 mrg Exp $

LIB=	usb
MAN=	usb.3

MLINKS=	usb.3 libusb.3 usb.3 hid_get_report_desc.3 \
	usb.3 hid_dispose_report_desc.3 \
	usb.3 hid_start_parse.3 usb.3 hid_end_parse.3 \
	usb.3 hid_get_item.3 usb.3 hid_report_size.3 usb.3 hid_locate.3 \
	usb.3 hid_usage_page.3 usb.3 hid_usage_in_page.3 usb.3 hid_init.3 \
	usb.3 hid_get_data.3 usb.3 hid_set_data.3

SRCS=	descr.c parse.c usage.c data.c

INCS=	usb.h
INCSDIR=/usr/include

.if ${MKSHARE} != "no"
FILES=  usb_hid_usages
FILESDIR=/usr/share/misc
.endif

.include <bsd.lib.mk>

