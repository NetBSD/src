# $NetBSD: Makefile,v 1.1 2003/12/22 13:16:07 rtr Exp $

XMODULEDIR=	${X11USRLIBDIR}/modules

.include "../../../Makefile.serverlib"

.include <bsd.own.mk>

LIB=	xaa
.PATH:	${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/${LIB}
SRCS=	lsb_fixed_xaaBitmap.c lsb_fixed_xaaBitmap3.c lsb_fixed_xaaStipple.c \
	lsb_fixed_xaaStipple3.c lsb_fixed_xaaTEGlyph.c lsb_xaaBitmap.c \
	lsb_xaaBitmap3.c lsb_xaaStipple.c lsb_xaaStipple3.c lsb_xaaTEGlyph.c \
	msb_fixed_xaaBitmap.c msb_fixed_xaaBitmap3.c msb_fixed_xaaStipple.c \
	msb_fixed_xaaStipple3.c msb_fixed_xaaTEGlyph.c msb_xaaBitmap.c \
	msb_xaaBitmap3.c msb_xaaStipple.c msb_xaaStipple3.c msb_xaaTEGlyph.c \
	xaaBitBlt.c xaaBitOrder.c xaaCpyArea.c xaaCpyPlane.c xaaCpyWin.c \
	xaaDashLine.c xaaDashSeg.c xaaFallback.c xaaFillArc.c xaaFillPoly.c \
	xaaFillRect.c xaaGC.c xaaGCmisc.c xaaImage.c xaaInit.c xaaInitAccel.c \
	xaaLine.c xaaLineMisc.c xaaNonTEText.c xaaOffscreen.c xaaOverlay.c \
	xaaOverlayDF.c xaaPCache.c xaaPaintWin.c xaaPict.c xaaROP.c xaaRect.c \
	xaaSeg.c xaaSpans.c xaaStateChange.c xaaTEText.c xaaTables.c \
	xaaWideLine.c

XSERVERINCDIR!= cd ${NETBSDSRCDIR}/x11/Xserver/include && ${PRINTOBJDIR}

CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}/X11 \
		-I${DESTDIR}${X11INCDIR}/X11/extensions \
		-I${X11SRCDIR.xc} \
		-I${X11SRCDIR.xc}/include/fonts \
		-I${X11SRCDIR.xc}/programs/Xserver/Xext \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/${LIB} \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/common \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/os-support \
		-I${X11SRCDIR.xc}/programs/Xserver/include \
		-I${X11SRCDIR.xc}/programs/Xserver/include/extensions \
		-I${X11SRCDIR.xc}/programs/Xserver/mi \
		-I${X11SRCDIR.xc}/programs/Xserver/render \
		-I${XSERVERINCDIR}

CPPFLAGS+=	${X11FLAGS.LOADABLE}

CPPFLAGS.lsb_fixed_xaaBitmap.c=		-DLSBFIRST -DFIXEDBASE
CPPFLAGS.lsb_fixed_xaaBitmap3.c=	-DLSBFIRST -DFIXEDBASE -DTRIPLE_BITS
CPPFLAGS.lsb_fixed_xaaStipple.c=	-DLSBFIRST -DFIXEDBASE
CPPFLAGS.lsb_fixed_xaaStipple3.c=	-DLSBFIRST -DFIXEDBASE -DTRIPLE_BITS
CPPFLAGS.lsb_fixed_xaaTEGlyph.c=	-DLSBFIRST -DFIXEDBASE
CPPFLAGS.lsb_xaaBitmap.c=		-DLSBFIRST
CPPFLAGS.lsb_xaaBitmap3.c=		-DLSBFIRST -DTRIPLE_BITS
CPPFLAGS.lsb_xaaStipple.c=		-DLSBFIRST
CPPFLAGS.lsb_xaaStipple3.c=		-DLSBFIRST -DTRIPLE_BITS
CPPFLAGS.lsb_xaaTEGlyph.c=		-DLSBFIRST
CPPFLAGS.msb_fixed_xaaBitmap.c=		-DMSBFIRST -DFIXEDBASE
CPPFLAGS.msb_fixed_xaaBitmap3.c=	-DMSBFIRST -DFIXEDBASE -DTRIPLE_BITS
CPPFLAGS.msb_fixed_xaaStipple.c=	-DMSBFIRST -DFIXEDBASE
CPPFLAGS.msb_fixed_xaaStipple3.c=	-DMSBFIRST -DFIXEDBASE -DTRIPLE_BITS
CPPFLAGS.msb_fixed_xaaTEGlyph.c=	-DMSBFIRST -DFIXEDBASE
CPPFLAGS.msb_xaaBitmap.c=		-DMSBFIRST
CPPFLAGS.msb_xaaBitmap3.c=		-DMSBFIRST -DTRIPLE_BITS
CPPFLAGS.msb_xaaStipple.c=		-DMSBFIRST
CPPFLAGS.msb_xaaStipple3.c=		-DMSBFIRST -DTRIPLE_BITS
CPPFLAGS.msb_xaaTEGlyph.c=		-DMSBFIRST
CPPFLAGS.xaaSeg.c=			-DPOLYSEGMENT
CPPFLAGS.xaaDashSeg.c=			-DPOLYSEGMENT

BUILDSYMLINKS= 	xaaBitmap.c lsb_xaaBitmap.c  \
		xaaBitmap.c lsb_fixed_xaaBitmap.c \
		xaaBitmap.c lsb_fixed_xaaBitmap3.c \
		xaaBitmap.c lsb_xaaBitmap3.c \
		xaaBitmap.c msb_fixed_xaaBitmap.c \
		xaaBitmap.c msb_fixed_xaaBitmap3.c \
		xaaBitmap.c msb_xaaBitmap.c \
		xaaBitmap.c msb_xaaBitmap3.c \
		xaaDashLine.c xaaDashSeg.c \
		xaaLine.c xaaSeg.c \
		xaaStipple.c lsb_fixed_xaaStipple.c \
		xaaStipple.c lsb_fixed_xaaStipple3.c \
		xaaStipple.c lsb_xaaStipple.c \
		xaaStipple.c lsb_xaaStipple3.c \
		xaaStipple.c msb_fixed_xaaStipple.c \
		xaaStipple.c msb_fixed_xaaStipple3.c \
		xaaStipple.c msb_xaaStipple.c \
		xaaStipple.c msb_xaaStipple3.c \
		xaaTEGlyph.c lsb_fixed_xaaTEGlyph.c \
		xaaTEGlyph.c lsb_xaaTEGlyph.c \
		xaaTEGlyph.c msb_fixed_xaaTEGlyph.c \
		xaaTEGlyph.c msb_xaaTEGlyph.c

.include <bsd.x11.mk>
LIBDIR=	${XMODULEDIR}
.include <bsd.lib.mk>
