#	$NetBSD: Makefile,v 1.1 2004/03/06 10:30:26 rtr Exp $

XMODULEDIR=	${X11ROOTDIR}/lib/modules/extensions

.include "../../Makefile.serverlib"

LIB=	dri

.PATH:	${X11SRCDIR.xc}/programs/Xserver/GL/dri
SRCS=	xf86dri.c dri.c drimodule.c

CPPFLAGS+=	-I${X11SRCDIR.xc}/programs/Xserver/include \
		-I${DESTDIR}${X11INCDIR}/X11 \
		-I${DESTDIR}${X11INCDIR}/X11/extensions \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/os-support \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/common \
		-I${X11SRCDIR.xc}/programs/Xserver/GL/include \
		-I${X11SRCDIR.xc}/programs/Xserver/GL/glx \
		-I${X11SRCDIR.xc}/lib/GL/include \
		-I${X11SRCDIR.xc}/lib/GL/dri \
		-I${X11SRCDIR.xc}/programs/Xserver/mi \
		-I${X11SRCDIR.xc}/include/fonts \
		-I${XSERVERINCDIR}

CPPFLAGS+=	${X11FLAGS.DRI}

.include <bsd.x11.mk>
LIBDIR=	${XMODULEDIR}
.include <bsd.lib.mk>

# XXX NOTICE!
#     subdirs install libxxx_dri.so.n.n however the final file installed
#     needs to be xxx_dri.so, since our .mk magic can't do this I've been
#     doing it by hand until I get enthusiastic about fixing it.
SUBDIR=	mga_dri radeon_dri

.include <bsd.subdir.mk>
