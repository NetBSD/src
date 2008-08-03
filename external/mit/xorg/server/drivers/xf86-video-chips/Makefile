#	$NetBSD: Makefile,v 1.3 2008/08/03 09:24:14 rtr Exp $

DRIVER=		xf86-video-chips
DRIVER_NAME=	chips_drv

SRCS=		ct_accel.c ct_bank.c ct_cursor.c ct_ddc.c ct_dga.c
SRCS+=		ct_driver.c ct_regs.c ct_shadow.c ct_video.c
MAN=		chips.4

CPPFLAGS.ct_driver.c=	-DVERSION=${PACKAGE_MAJOR}

.include "../Makefile.xf86-driver"
