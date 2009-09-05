#	$NetBSD: Makefile,v 1.1 2009/09/05 22:35:53 macallan Exp $

DRIVER=		xf86-video-sunleo
DRIVER_NAME=	sunleo_drv

SRCS=		leo_accel.c leo_checks.c leo_cursor.c leo_driver.c \
		leo_frect.c leo_frectsp.c leo_fspans.c leo_fspanssp.c \
		leo_glyph.c

CPPFLAGS+=	${X11FLAGS.DRI}

MAN=		sunleo.4

.include "../Makefile.xf86-driver"
