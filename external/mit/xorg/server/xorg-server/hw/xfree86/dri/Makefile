# $NetBSD: Makefile,v 1.1 2008/08/05 05:58:52 mrg Exp $

#XMODULEDIR=	${X11USRLIBDIR}/modules

.include "../../../Makefile.serverlib"
.include "../../../Makefile.servermod"

LIB=	dri

.PATH:	${X11SRCDIR.xorg-server}/hw/xfree86/${LIB}
SRCS=	dri.c drimodule.c xf86dri.c

CPPFLAGS+=	-I${X11SRCDIR.xorg-server}/hw/xfree86/${LIB} \
		-I${X11SRCDIR.xorg-server}/include \
		-I${X11SRCDIR.xorg-server}/hw/xfree86/common \
		-I${X11SRCDIR.xorg-server}/hw/xfree86/os-support \
		-I${X11SRCDIR.xorg-server}/hw/xfree86/os-support/bus \
		-I${X11SRCDIR.xorg-server}/GL/glx \
		-I${X11SRCDIR.xorg-server}/GL/include \
		-I${X11SRCDIR.MesaLib}/include \
		-I${DESTDIR}${X11INCDIR}/X11 \
		-I${DESTDIR}${X11INCDIR}/X11/dri \
		-I${DESTDIR}${X11INCDIR}/X11/drm \
		-I${DESTDIR}${X11INCDIR}/pixman-1 \
		-I${DESTDIR}${X11INCDIR}/xorg \
		-I${XSERVERINCDIR}

CPPFLAGS+=	-DXORG_VERSION_CURRENT=${XORG_VERSION_CURRENT} \
		-DXF86DRI \
		${X11FLAGS.DIX}

CPPFLAGS.dri.c=	-Wno-error

.include <bsd.x11.mk>
LIBDIR=	${XMODULEDIR}
.include <bsd.lib.mk>
