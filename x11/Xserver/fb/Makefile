#	$NetBSD: Makefile,v 1.2 2003/09/23 23:21:11 lukem Exp $

XMODULEDIR=	${X11ROOTDIR}/lib/modules

.include "../Makefile.serverlib"

.include <bsd.own.mk>

LIB=		fb
.PATH:		${X11SRCDIR.xc}/programs/Xserver/fb

SRCS=		fb24_32.c fballpriv.c fbarc.c fbbits.c fbblt.c \
		fbbltone.c fbbstore.c fbcmap.c fbcompose.c fbcopy.c \
		fbfill.c fbfillrect.c fbfillsp.c fbgc.c fbgetsp.c \
		fbglyph.c fbimage.c fbline.c fbmodule.c fboverlay.c \
		fbpict.c fbpixmap.c fbpoint.c fbpush.c fbscreen.c \
		fbseg.c fbsetsp.c fbsolid.c fbstipple.c fbtile.c \
		fbtrap.c fbutil.c fbwindow.c

XSERVERINCDIR!=	cd ${NETBSDSRCDIR}/x11/Xserver/include && ${PRINTOBJDIR}

CPPFLAGS+=	-I${X11SRCDIR.xc}/programs/Xserver/fb \
		-I${X11SRCDIR.xc}/programs/Xserver/mi \
		-I${X11SRCDIR.xc}/programs/Xserver/include \
		-I${XSERVERINCDIR} \
		-I${DESTDIR}/${X11INCDIR}/X11 \
		-I${X11SRCDIR.xc}/include/fonts \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/common \
		-I${X11SRCDIR.xc}/programs/Xserver/render \
		-I${DESTDIR}/${X11INCDIR}/X11/extensions \
		-I${X11SRCDIR.xc}/programs/Xserver/Xext

.include <bsd.x11.mk>
LIBDIR=	${XMODULEDIR}
.include <bsd.lib.mk>
