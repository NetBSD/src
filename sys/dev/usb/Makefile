#	$NetBSD: Makefile,v 1.9.154.1 2018/09/06 06:56:04 pgoyette Exp $

# use 'make -f Makefile.usbdevs' to make usbdevs.h and usbdevs_data.h
# _after_ you committed usbdevs. See comment in Makefile.usbdevs

INCSDIR= /usr/include/dev/usb

# Only install includes which are used by userland
INCS=	if_umbreg.h mbim.h ukyopon.h urio.h usb.h usbhid.h utoppy.h

.include <bsd.kinc.mk>
