#	$NetBSD: Makefile,v 1.3 2008/08/03 09:24:14 rtr Exp $

DRIVER=		xf86-video-savage
DRIVER_NAME=	savage_drv

SRCS=		savage_accel.c savage_xaa.c savage_exa.c
SRCS+=		savage_cursor.c savage_dga.c savage_driver.c
SRCS+=		savage_i2c.c savage_shadow.c savage_streams.c
SRCS+=		savage_vbe.c savage_video.c
MAN=		savage.4

CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}/X11
CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}/X11/dri
CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}/X11/drm

CPPFLAGS.savage_driver.c=	-DPACKAGE_VERSION=\"${PACKAGE_MAJOR}\"
CPPFLAGS.savage_driver.c+=	-DPACKAGE_VERSION_MAJOR=${PACKAGE_MAJOR}
CPPFLAGS.savage_driver.c+=	-DPACKAGE_VERSION_MINOR=${PACKAGE_MINOR}
CPPFLAGS.savage_driver.c+=	-DPACKAGE_VERSION_PATCHLEVEL=${PACKAGE_PATCH}

.include "../Makefile.xf86-driver"
