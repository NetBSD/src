#	$NetBSD: Makefile,v 1.4.8.2 2009/09/17 04:24:36 snj Exp $

DRIVER=		xf86-video-savage
DRIVER_NAME=	savage_drv

SRCS=	\
	savage_accel.c \
	savage_xaa.c \
	savage_exa.c \
	savage_cursor.c \
	savage_dga.c \
	savage_driver.c \
	savage_i2c.c \
	savage_shadow.c \
	savage_streams.c \
	savage_vbe.c \
	savage_video.c \
	savage_hwmc.c \
	savage_dri.c
MAN=	savage.4

CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}/X11
CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}/X11/dri
CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}/X11/drm

CPPFLAGS.savage_driver.c=	-DPACKAGE_VERSION=\"${PACKAGE_MAJOR}\"
CPPFLAGS.savage_exa.c=		-Wno-error	# volatile -> mempcy

.include "../Makefile.xf86-driver"
