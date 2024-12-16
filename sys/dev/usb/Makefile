#	$NetBSD: Makefile,v 1.13 2024/12/16 16:37:38 brad Exp $

# use 'make -f Makefile.usbdevs' to make usbdevs.h and usbdevs_data.h
# _after_ you committed usbdevs. See comment in Makefile.usbdevs

INCSDIR= /usr/include/dev/usb

# Only install includes which are used by userland
INCS=	if_umbreg.h mbim.h ukyopon.h umcpmio_io.h umcpmio_hid_reports.h \
	usb.h usbhid.h utoppy.h vhci.h

.include <bsd.kinc.mk>
