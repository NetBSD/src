#	$NetBSD: Makefile,v 1.3 2010/05/22 23:04:01 mrg Exp $

DRIVER=		xf86-video-r128
DRIVER_NAME=	r128_drv

SRCS=	r128_accel.c r128_cursor.c r128_dga.c r128_driver.c \
	r128_video.c r128_misc.c r128_probe.c r128_dri.c

MAN=	r128.4

# Enable VGA support (esp. hw state save/restore) on i386 / amd64 platforms
.if ${MACHINE} == "amd64" || ${MACHINE} == "i386"
CPPFLAGS+=	-DWITH_VGAHW
.endif

CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}/libdrm

.include "../Makefile.xf86-driver"
