#	$NetBSD: Makefile,v 1.2.2.1 2004/04/16 07:50:34 tron Exp $

NOMAN=		1

.include <bsd.own.mk>

HOSTPROG=	mkfontscale

.PATH:	${X11SRCDIR.xc}/programs/mkfontscale
SRCS.mkfontscale=	mkfontscale.c list.c hash.c ident.c

FREETYPETOP=	${X11SRCDIR.xc}/extras/freetype2
FREETYPESRC=	${FREETYPETOP}/src

#.PATH:	${FREETYPESRC}
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
SRCS.freetype=	ftapi.c ftbase.c ftbbox.c ftbdf.c ftdebug.c ftglyph.c \
		ftinit.c ftmm.c ftpfr.c ftstroker.c ftsynth.c ftsystem.c \
		fttype1.c ftwinfnt.c ftxf86.c autohint.c bdf.c cff.c \
		type1cid.c ftgzip.c pcf.c pfr.c psaux.c pshinter.c psnames.c \
		raster.c sfnt.c smooth.c truetype.c type1.c type42.c winfnt.c

.PATH:	${X11SRCDIR.xc}/lib/font/fontfile
.PATH:	${X11SRCDIR.xc}/xc/lib/font/include
SRCS.fontenc=	encparse.c fontenc.c

# XXX we should be declaring some of these on a per <fn> basis but
# XXX HOST_CPPFLAGS doesn't support it we could CPPFLAGS= and then
# XXX use CPPFLAGS but thats lame
HOST_CPPFLAGS+=	-DFONTENC_NO_LIBFONT \
		${X11FLAGS.THREADLIB} \
		-DFREETYPE2 -DXFREE86_FT2 \
		-DFT_CONFIG_OPTION_SYSTEM_ZLIB \
		-DFONT_ENCODINGS_DIRECTORY=\"${X11FONTDIR}/encodings/encodings.dir\"
HOST_CPPFLAGS+=	-I${DESTDIR}${X11INCDIR} \
		-I${DESTDIR}${X11INCDIR}/freetype2 \
		-I${DESTDIR}${X11INCDIR}/X11/fonts \
		-I${X11SRCDIR.xc}/lib/font/include \
		-I${X11SRCDIR.xc}/include/fonts

# This tells a cross build on darwin not to bother with
# resource fork font support which needs the carbon libraries.
HOST_CPPFLAGS+=	-DDARWIN_NO_CARBON

SRCS=	${SRCS.mkfontscale} ${SRCS.freetype} ${SRCS.fontenc}

LDADD=	-lz
DPADD=	${LIBZ}

.include <bsd.x11.mk>
.include <bsd.hostprog.mk>
