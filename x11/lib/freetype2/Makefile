#	$NetBSD: Makefile,v 1.5 2004/03/17 23:15:30 perry Exp $

.include <bsd.own.mk>

LIB=	freetype

FREETYPETOP=	${X11SRCDIR.xc}/extras/freetype2
FREETYPESRC=	${FREETYPETOP}/src

SUBDIR=	freetype

SRCS=	ftapi.c ftbase.c ftbbox.c ftbdf.c ftdebug.c ftglyph.c \
	ftinit.c ftmm.c ftpfr.c ftstroker.c ftsynth.c ftsystem.c \
	fttype1.c ftwinfnt.c ftxf86.c autohint.c bdf.c cff.c \
	type1cid.c ftgzip.c pcf.c pfr.c psaux.c pshinter.c psnames.c \
	raster.c sfnt.c smooth.c truetype.c type1.c type42.c winfnt.c

INCS=	ft2build.h
INCSDIR=${X11INCDIR}/freetype2
.PATH:	${FREETYPETOP}/include

CPPFLAGS+=	${X11FLAGS.THREADLIB}
CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}/freetype2

.if defined(MKTTINTERP) && ${MKTTINTERP} != "no"
CPPFLAGS+=	-DTT_CONFIG_OPTION_BYTECODE_INTERPRETER
.endif

.PATH:	${FREETYPESRC}/autohint
.PATH:	${FREETYPESRC}/base
.PATH:	${FREETYPESRC}/bdf
.PATH:	${FREETYPESRC}/cff
.PATH:	${FREETYPESRC}/cid
.PATH:	${FREETYPESRC}/gzip
.PATH:	${FREETYPESRC}/pcf
.PATH:	${FREETYPESRC}/pfr
.PATH:	${FREETYPESRC}/psaux
.PATH:	${FREETYPESRC}/pshinter
.PATH:	${FREETYPESRC}/psnames
.PATH:	${FREETYPESRC}/raster
.PATH:	${FREETYPESRC}/sfnt
.PATH:	${FREETYPESRC}/smooth
.PATH:	${FREETYPESRC}/truetype
.PATH:	${FREETYPESRC}/type1
.PATH:	${FREETYPESRC}/type42
.PATH:	${FREETYPESRC}/winfonts

.include <bsd.x11.mk>
.include <bsd.lib.mk>
.include <bsd.subdir.mk>
