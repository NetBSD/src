#	$NetBSD: Makefile,v 1.7.2.1 2004/11/15 09:12:46 rtr Exp $

.include <bsd.own.mk>

LIB=	freetype

FREETYPE=	${X11SRCDIR.xc}/extras/freetype2

SUBDIR=	freetype

.PATH:	${FREETYPE}/src/base
SRCS=	ftapi.c ftbase.c ftbbox.c ftbdf.c ftdebug.c ftglyph.c \
	ftinit.c ftmm.c ftpfr.c ftstroke.c ftsynth.c ftsystem.c \
	fttype1.c ftwinfnt.c ftxf86.c

.PATH:	${FREETYPE}/src/autofit
SRCS+=	autofit.c

.PATH:	${FREETYPE}/src/autohint
SRCS+=	autohint.c

.PATH:	${FREETYPE}/src/bdf
SRCS+=	bdf.c

.PATH:	${FREETYPE}/src/cff
SRCS+=	cff.c

.PATH:	${FREETYPE}/src/cid
SRCS+=	type1cid.c

.PATH:	${FREETYPE}/src/gzip
SRCS+=	ftgzip.c

.PATH:	${FREETYPE}/src/lzw
SRCS+=	ftlzw.c

.PATH:	${FREETYPE}/src/pcf
SRCS+=	pcf.c

.PATH:	${FREETYPE}/src/pfr
SRCS+=	pfr.c

.PATH:	${FREETYPE}/src/psaux
SRCS+=	psaux.c

.PATH:	${FREETYPE}/src/pshinter
SRCS+=	pshinter.c

.PATH:	${FREETYPE}/src/psnames
SRCS+=	psnames.c

.PATH:	${FREETYPE}/src/raster
SRCS+=	raster.c

.PATH:	${FREETYPE}/src/sfnt
SRCS+=	sfnt.c

.PATH:	${FREETYPE}/src/smooth
SRCS+=	smooth.c

.PATH:	${FREETYPE}/src/truetype
SRCS+=	truetype.c

.PATH:	${FREETYPE}/src/type1
SRCS+=	type1.c

.PATH:	${FREETYPE}/src/type42
SRCS+=	type42.c

.PATH:	${FREETYPE}/src/winfonts
SRCS+=	winfnt.c

.PATH:	${FREETYPE}/include
INCS=	ft2build.h
INCSDIR=${X11INCDIR}

CPPFLAGS+=	${X11FLAGS.THREADLIB} -DXFREE86_FT2
CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}/freetype2 \
		-I${X11SRCDIR.xc}/extras/freetype2/include

.if defined(MKTTINTERP) && ${MKTTINTERP} != "no"
CPPFLAGS+=	-DTT_CONFIG_OPTION_BYTECODE_INTERPRETER
.endif

.include <bsd.x11.mk>
.include <bsd.lib.mk>
.include <bsd.subdir.mk>
