#	$NetBSD: Makefile,v 1.4.2.1 2004/11/15 09:12:47 rtr Exp $

NOMAN=		1

.include <bsd.own.mk>

HOSTPROG=	mkfontscale

FREETYPE=       ${X11SRCDIR.xc}/extras/freetype2


.PATH:	${X11SRCDIR.xc}/programs/mkfontscale
SRCS.mkfontscale=	mkfontscale.c list.c hash.c ident.c


.PATH:	${FREETYPE}/src/base
SRCS.freetype+=	ftapi.c ftbase.c ftbbox.c ftbdf.c ftdebug.c ftglyph.c \
		ftinit.c ftmm.c ftpfr.c ftstroke.c ftsynth.c ftsystem.c \
		fttype1.c ftwinfnt.c ftxf86.c

.PATH: ${FREETYPE}/src/autofit
SRCS.freetype+=	autofit.c

.PATH:	${FREETYPE}/src/autohint
SRCS.freetype+=	autohint.c

.PATH:	${FREETYPE}/src/bdf
SRCS.freetype+=	bdf.c

.PATH:	${FREETYPE}/src/cff
SRCS.freetype+=	cff.c

.PATH:	${FREETYPE}/src/cid
SRCS.freetype+=	type1cid.c

.PATH:	${FREETYPE}/src/gzip
SRCS.freetype+=	ftgzip.c

.PATH:	${FREETYPE}/src/lzw
SRCS.freetype+=	ftlzw.c

.PATH:	${FREETYPE}/src/pcf
SRCS.freetype+=	pcf.c

.PATH:	${FREETYPE}/src/pfr
SRCS.freetype+=	pfr.c

.PATH:	${FREETYPE}/src/psaux
SRCS.freetype+=	psaux.c

.PATH:	${FREETYPE}/src/pshinter
SRCS.freetype+=	pshinter.c

.PATH:	${FREETYPE}/src/psnames
SRCS.freetype+=	psnames.c

.PATH:	${FREETYPE}/src/raster
SRCS.freetype+=	raster.c

.PATH:	${FREETYPE}/src/sfnt
SRCS.freetype+=	sfnt.c

.PATH:	${FREETYPE}/src/smooth
SRCS.freetype+=	smooth.c

.PATH:	${FREETYPE}/src/truetype
SRCS.freetype+=	truetype.c

.PATH:	${FREETYPE}/src/type1
SRCS.freetype+=	type1.c

.PATH:	${FREETYPE}/src/type42
SRCS.freetype+=	type42.c

.PATH:	${FREETYPE}/src/winfonts
SRCS.freetype+=	winfnt.c

.PATH:	${X11SRCDIR.xc}/lib/font/fontfile
SRCS.fontenc=	fontenc.c encparse.c

SRCS+=	${SRCS.mkfontscale} ${SRCS.freetype} ${SRCS.fontenc}

LDADD=	-lz

HOST_CPPFLAGS=	-DFONTENC_NO_LIBFONT -DXFREE86_FT2 -DFONTENC_NO_LIBFONT \
		-DFONT_ENCODINGS_DIRECTORY=\"${X11FONTDIR}/encodings/encodings.dir\"
HOST_CPPFLAGS+=	-I${FREETYPE}/include -I${DESTDIR}${X11INCDIR} \
		-I${X11SRCDIR.xc}/lib/font/include \
		-I${X11SRCDIR.xc}/include/fonts


.include <bsd.x11.mk>
.include <bsd.hostprog.mk>
