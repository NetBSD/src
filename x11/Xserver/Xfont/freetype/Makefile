#	$NetBSD: Makefile,v 1.5 2004/03/17 23:15:30 perry Exp $

XMODULEDIR=     ${X11USRLIBDIR}/modules/fonts

.include "../../Makefile.serverlib"

LIB=	freetype

FT2DIR=	${X11SRCDIR.xc}/extras/freetype2
FT2SRC=	${FT2DIR}/src

.PATH:	${X11SRCDIR.xc}/lib/font/FreeType
SRCS=	xttcap.c ftfuncs.c ftenc.c fttools.c

.PATH:	${X11SRCDIR.xc}/lib/font/FreeType/module
SRCS+=	ftmodule.c

.PATH:	${FT2SRC}/base
SRCS+=	ftbase.c ftinit.c ftglyph.c ftbbox.c ftsystem.c fttype1.c ftxf86.c ftbdf.c 
	
.PATH:	${FT2SRC}/gzip
SRCS+=	ftgzip.c

.PATH:	${FT2SRC}/raster
SRCS+=	raster.c

.PATH:	${FT2SRC}/sfnt
SRCS+=	sfnt.c

.PATH:	${FT2SRC}/truetype
SRCS+=	truetype.c

.PATH:	${FT2SRC}/cff
SRCS+=	cff.c

.PATH:	${FT2SRC}/type1
SRCS+=	type1.c

.PATH:	${FT2SRC}/psaux
SRCS+=	psaux.c

.PATH:	${FT2SRC}/psnames
SRCS+=	psnames.c

.PATH:	${FT2SRC}/pshinter
SRCS+=	pshinter.c

.PATH:	${FT2SRC}/autohint
SRCS+=	autohint.c

.PATH:	${FT2SRC}/bdf
SRCS+=	bdf.c

.PATH:	${FT2SRC}/pcf
SRCS+=	pcf.c


CPPFLAGS+=	-I${X11SRCDIR.xc}/lib/font/FreeType/module \
		-I${X11SRCDIR.xc}/lib/font/FreeType \
		-I${X11SRCDIR.xc}/include/fonts \
		-I${X11SRCDIR.xc}/lib/font/include \
		-I${DESTDIR}${X11INCDIR}/X11 \
		-I${X11SRCDIR.xc}/programs/Xserver/include \
		-I${FT2DIR}/include \
		-I${FT2SRC}/base \
		-I${FT2SRC}/gzip \
		-I${FT2SRC}/raster \
		-I${FT2SRC}/sfnt \
		-I${FT2SRC}/truetype \
		-I${FT2SRC}/cff \
		-I${FT2SRC}/type1 \
		-I${FT2SRC}/psaux \
		-I${FT2SRC}/psnames \
		-I${FT2SRC}/pshinter \
		-I${FT2SRC}/autohint \
		-I${FT2SRC}/bdf \
		-I${FT2SRC}/pcf \
		-I${XSERVERINCDIR}

CPPFLAGS+=	-DXFREE86_FT2

.include <bsd.x11.mk>

.if defined(MKTTINTERP) && ${MKTTINTERP} != "no"
CPPFLAGS+=	-DTT_CONFIG_OPTION_BYTECODE_INTERPRETER
.endif

.if !empty(X11FLAGS.LOADABLE)
CPPFLAGS+=	-DFONTMODULE
LIBDIR=	${XMODULEDIR}
.else
libinstall::
.endif

.include <bsd.lib.mk>
