#	$NetBSD: Makefile,v 1.1.1.1 2003/09/12 02:50:54 lukem Exp $

.include <bsd.own.mk>

LIB=	freetype

FREETYPETOP=	${X11SRCDIR.xc}/extras/freetype2
FREETYPESRC=	${FREETYPETOP}/src

SUBDIR=	freetype

SRCS=	autohint.c bdf.c cff.c ftbase.c ftbbox.c ftbdf.c ftdebug.c \
	ftglyph.c ftinit.c ftmm.c ftsynth.c ftsystem.c fttype1.c \
	ftxf86.c pcf.c pfr.c psaux.c pshinter.c psnames.c raster.c \
	sfnt.c smooth.c truetype.c type1.c type1cid.c type42.c winfnt.c

INCS=	ft2build.h
INCSDIR=${X11INCDIR}/freetype2
.PATH:	${FREETYPETOP}/include

CPPFLAGS+=	${X11FLAGS.THREADLIB}
CPPFLAGS+=	-I${DESTDIR}/${X11INCDIR}/freetype2

.PATH:	${FREETYPESRC}/autohint
.PATH:	${FREETYPESRC}/base
.PATH:	${FREETYPESRC}/bdf
.PATH:	${FREETYPESRC}/cff
.PATH:	${FREETYPESRC}/cid
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

.include <bsd.lib.mk>
.include <bsd.subdir.mk>
.include <bsd.x11.mk>
