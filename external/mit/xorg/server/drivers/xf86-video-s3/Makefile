#	$NetBSD: Makefile,v 1.3.6.2 2008/10/19 22:41:16 haad Exp $

DRIVER=		xf86-video-s3
DRIVER_NAME=	s3_drv

SRCS=		s3_bios.c s3_cursor.c s3_dga.c s3_driver.c s3_IBMRGB.c
SRCS+=		s3_Ti.c s3_Trio64DAC.c s3_video.c

CPPFLAGS.s3_driver.c=	-Wno-error	# XXX fix me please
.include "../Makefile.xf86-driver"
