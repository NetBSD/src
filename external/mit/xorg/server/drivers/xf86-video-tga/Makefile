#	$NetBSD: Makefile,v 1.8 2016/08/16 07:10:01 mrg Exp $

DRIVER=		xf86-video-tga
DRIVER_NAME=	tga_drv

SRCS=		BT463ramdac.c BTramdac.c IBM561ramdac.c ICS1562.c
SRCS+=		tga_accel.c tga_cursor.c tga_dac.c tga_driver.c
SRCS+=		tga_line.c tga_seg.c

# XXX
COPTS.BT463ramdac.c+=	-Wno-error
COPTS.BTramdac.c+=	-Wno-error
COPTS.IBM561ramdac.c+=	-Wno-error
COPTS.tga_accel.c+=	-Wno-error
COPTS.tga_cursor.c+=	-Wno-error
COPTS.tga_dac.c+=	-Wno-error
COPTS.tga_driver.c+=	-Wno-error

.include "../Makefile.xf86-driver"
