#	$NetBSD: Makefile,v 1.1.4.1 2004/11/21 10:01:26 rtr Exp $

XMODULEDIR=	${X11USRLIBDIR}/modules/extensions

.include "../../../Makefile.serverlib"
.include "../../../Makefile.servermod"

LIB=	drm

.PATH:	${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/os-support/linux/drm
SRCS=	xf86drm.c xf86drmHash.c xf86drmRandom.c xf86drmSL.c drmmodule.c 

CPPFLAGS+=	-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/common \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/os-support \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/os-support/bsd/drm \
		-I${X11SRCDIR.xc}/programs/Xserver/include \
		-I${DESTDIR}${X11INCDIR}/X11 \
		-I${DESTDIR}${X11INCDIR}/extensions \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/os-support/bsd/drm/kernel \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/os-support/shared/drm/kernel \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/os-support/linux/drm/kernel \
		-I${XSERVERINCDIR}

CPPFLAGS+=	-DHAS_MTRR_SUPPORT ${X11FLAGS.DRI} ${X11FLAGS.SERVER}

.include <bsd.x11.mk>
LIBDIR=	${XMODULEDIR}
.include <bsd.lib.mk>

SUBDIR=	mga #radeon

.include <bsd.subdir.mk>
