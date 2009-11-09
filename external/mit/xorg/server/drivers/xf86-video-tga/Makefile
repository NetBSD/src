#	$NetBSD: Makefile,v 1.4 2009/11/09 02:22:39 mrg Exp $

DRIVER=		xf86-video-tga
DRIVER_NAME=	tga_drv

SRCS=		BT463ramdac.c BTramdac.c IBM561ramdac.c ICS1562.c
SRCS+=		tga_accel.c tga_cursor.c tga_dac.c tga_driver.c
SRCS+=		tga_line.c tga_seg.c

# new xextproto
#CPPFLAGS+=	-DHAVE_XEXTPROTO_71

.include "../Makefile.xf86-driver"
