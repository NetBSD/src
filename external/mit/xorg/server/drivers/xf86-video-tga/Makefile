#	$NetBSD: Makefile,v 1.5 2010/05/22 07:26:09 mrg Exp $

DRIVER=		xf86-video-tga
DRIVER_NAME=	tga_drv

SRCS=		BT463ramdac.c BTramdac.c IBM561ramdac.c ICS1562.c
SRCS+=		tga_accel.c tga_cursor.c tga_dac.c tga_driver.c
SRCS+=		tga_line.c tga_seg.c

# new xextproto
CPPFLAGS+=	-DHAVE_XEXTPROTO_71

COPTS.tga_driver.c=	-Wno-error	# XXX xf86dgastr.h

.include "../Makefile.xf86-driver"
