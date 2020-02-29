#	$NetBSD: Makefile,v 1.10.8.1 2020/02/29 20:19:16 ad Exp $

# use 'make -f Makefile.usbdevs' to make usbdevs.h and usbdevs_data.h
# _after_ you committed usbdevs. See comment in Makefile.usbdevs

INCSDIR= /usr/include/dev/usb

# Only install includes which are used by userland
INCS=	if_umbreg.h mbim.h ukyopon.h usb.h usbhid.h utoppy.h

.include <bsd.kinc.mk>
