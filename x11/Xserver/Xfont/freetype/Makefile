#	$NetBSD: Makefile,v 1.1 2003/12/23 10:04:14 rtr Exp $

XMODULEDIR=     ${X11USRLIBDIR}/modules/fonts

.include "../../Makefile.serverlib"

.include <bsd.own.mk>

LIB=	freetype
.PATH:	${X11SRCDIR.xc}/lib/font/FreeType
.PATH:	${X11SRCDIR.xc}/lib/font/FreeType/module
.PATH:	${X11SRCDIR.xc}/extras/freetype2/src/autohint
.PATH:	${X11SRCDIR.xc}/extras/freetype2/src/cff
.PATH:	${X11SRCDIR.xc}/extras/freetype2/src/base
.PATH:	${X11SRCDIR.xc}/extras/freetype2/src/psaux
.PATH:	${X11SRCDIR.xc}/extras/freetype2/src/pshinter
.PATH:	${X11SRCDIR.xc}/extras/freetype2/src/psnames
.PATH:	${X11SRCDIR.xc}/extras/freetype2/src/raster
.PATH:	${X11SRCDIR.xc}/extras/freetype2/src/sfnt
.PATH:	${X11SRCDIR.xc}/extras/freetype2/src/truetype
.PATH:	${X11SRCDIR.xc}/extras/freetype2/src/type1

SRCS=	ftfuncs.c ftenc.c fttools.c ftbase.c ftinit.c ftglyph.c ftsystem.c \
	fttype1.c ftxf86.c raster.c sfnt.c truetype.c cff.c type1.c \
	psaux.c psnames.c pshinter.c  autohint.c ftmodule.c

XSERVERINCDIR!= cd ${NETBSDSRCDIR}/x11/Xserver/include && ${PRINTOBJDIR}

CPPFLAGS+=	-I${X11SRCDIR.xc}/lib/font/FreeType \
		-I${X11SRCDIR.xc}/include/fonts \
		-I${X11SRCDIR.xc}/lib/font/include \
		-I${DESTDIR}${X11INCDIR}/X11 \
		-I${X11SRCDIR.xc}/programs/Xserver/include \
		-I${X11SRCDIR.xc}/extras/freetype2/include \
		-I${X11SRCDIR.xc}/extras/freetype2/src/base \
		-I${X11SRCDIR.xc}/extras/freetype2/src/raster \
		-I${X11SRCDIR.xc}/extras/freetype2/src/sfnt \
		-I${X11SRCDIR.xc}/extras/freetype2/src/truetype \
		-I${X11SRCDIR.xc}/extras/freetype2/src/cff \
		-I${X11SRCDIR.xc}/extras/freetype2/src/type1 \
		-I${X11SRCDIR.xc}/extras/freetype2/src/psaux \
		-I${X11SRCDIR.xc}/extras/freetype2/src/psnames \
		-I${X11SRCDIR.xc}/extras/freetype2/src/pshinter \
		-I${X11SRCDIR.xc}/extras/freetype2/src/autohint \
		-I${XSERVERINCDIR}

CPPFLAGS+=	-DFONTMODULE

.include <bsd.x11.mk>
LIBDIR=	${XMODULEDIR}
.include <bsd.lib.mk>
