#	$NetBSD: Makefile,v 1.12 2020/03/31 16:28:28 maxv Exp $

# use 'make -f Makefile.usbdevs' to make usbdevs.h and usbdevs_data.h
# _after_ you committed usbdevs. See comment in Makefile.usbdevs

INCSDIR= /usr/include/dev/usb

# Only install includes which are used by userland
INCS=	if_umbreg.h mbim.h ukyopon.h usb.h usbhid.h utoppy.h vhci.h

.include <bsd.kinc.mk>
