#	$NetBSD: Makefile,v 1.10 2018/07/31 16:44:29 khorben Exp $

# use 'make -f Makefile.usbdevs' to make usbdevs.h and usbdevs_data.h
# _after_ you committed usbdevs. See comment in Makefile.usbdevs

INCSDIR= /usr/include/dev/usb

# Only install includes which are used by userland
INCS=	if_umbreg.h mbim.h ukyopon.h urio.h usb.h usbhid.h utoppy.h

.include <bsd.kinc.mk>
