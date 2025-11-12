#	$NetBSD: Makefile,v 1.4 2025/11/12 08:41:38 mrg Exp $

XMODULEDIR=	${X11USRLIBDIR}/modules

.include "../../../Makefile.serverlib"
.include "../../../Makefile.servermod"

LIB=	glamoregl

.PATH:	${X11SRCDIR.xorg-server}/hw/xfree86/glamor_egl
SRCS=	glamor_xf86_xv.c

.include "../../../glamor/Makefile.glamor"

CPPFLAGS+=	\
	-I${X11SRCDIR.xorg-server}/hw/xfree86/common \
	-I${X11SRCDIR.xorg-server}/Xext \
	-I${X11SRCDIR.xorg-server}/mi \
	-I${X11SRCDIR.xorg-server}/include \
	-I${X11SRCDIR.xorg-server}/glamor \
	-I${X11SRCDIR.xorg-server}/glx \
	-I${X11SRCDIR.xorg-server}/composite \
	-I${X11SRCDIR.xorg-server}/damageext \
	-I${X11SRCDIR.xorg-server}/xfixes \
	-I${X11SRCDIR.xorg-server}/Xi \
	-I${X11SRCDIR.xorg-server}/../include \
	-I${DESTDIR}${X11INCDIR}/X11 \
	-I${DESTDIR}${X11INCDIR}/pixman-1 \
	-I${DESTDIR}${X11INCDIR}/xorg \
	-I${DESTDIR}${X11INCDIR}/libdrm \
	-I${X11SRCDIR.xorg-server}/hw/xfree86/ddc \
	-I${X11SRCDIR.xorg-server}/hw/xfree86/i2c

CPPFLAGS+=	\
	-DGLAMOR_HAS_GBM

.include "../../../../../lib/mesa-which.mk"

LIBDPLIBS+=	\
	gbm	${.CURDIR}/../../../../../lib/libgbm${OLD_SUFFIX} \
	glx	${.CURDIR}/../../../hw/xfree86/dixmods/glx

.include <bsd.x11.mk>
LIBDIR=	${XMODULEDIR}
.include <bsd.lib.mk>
