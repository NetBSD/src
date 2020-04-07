#	$NetBSD: Makefile,v 1.11 2020/01/29 21:52:16 mlelstv Exp $

# use 'make -f Makefile.usbdevs' to make usbdevs.h and usbdevs_data.h
# _after_ you committed usbdevs. See comment in Makefile.usbdevs

INCSDIR= /usr/include/dev/usb

# Only install includes which are used by userland
INCS=	if_umbreg.h mbim.h ukyopon.h usb.h usbhid.h utoppy.h

.include <bsd.kinc.mk>
