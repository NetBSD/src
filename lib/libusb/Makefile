#	$NetBSD: Makefile,v 1.2 1999/05/12 00:14:58 augustss Exp $

LIB=	usb
MAN=	usb.3

MLINKS=	usb.3 libusb.3 usb.3 get_report_desc.3 usb.3 dispose_report_desc.3 \
	usb.3 hid_start_parse.3 usb.3 hid_end_parse.3 \
	usb.3 hid_get_item.3 usb.3 hid_report_size.3 usb.3 hid_locate.3 \
	usb.3 usage_page.3 usb.3 usage_in_page.3 usb.3 init_hid.3 \
	usb.3 get_data.3 usb.3 set_data.3

SRCS=	descr.c parse.c usage.c data.c

INCS=	usb.h
INCSDIR=/usr/include

FILES=  usb_hid_usages
FILESDIR=/usr/share/misc

.include <bsd.lib.mk>

