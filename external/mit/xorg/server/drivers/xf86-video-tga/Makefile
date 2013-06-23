#	$NetBSD: Makefile,v 1.6.12.1 2013/06/23 06:28:47 tls Exp $

DRIVER=		xf86-video-tga
DRIVER_NAME=	tga_drv

SRCS=		BT463ramdac.c BTramdac.c IBM561ramdac.c ICS1562.c
SRCS+=		tga_accel.c tga_cursor.c tga_dac.c tga_driver.c
SRCS+=		tga_line.c tga_seg.c

CPPFLAGS+=	-DHAVE_XAA_H

COPTS.tga_driver.c=	-Wno-error	# XXX xf86dgastr.h

.include "../Makefile.xf86-driver"
