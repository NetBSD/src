#	$NetBSD: Makefile,v 1.5 2018/08/03 02:23:09 kamil Exp $

.include <bsd.own.mk>

LIB=	Xfont2
XFONT2DIR= ${X11SRCDIR.${LIB}}

SHLIB_MAJOR=	2
SHLIB_MINOR=	0

.PATH:	${XFONT2DIR}/include/X11/fonts
.PATH:	${XFONT2DIR}/../include

SRCS.stubs=	\
	atom.c \
	libxfontstubs.c
.PATH:	${XFONT2DIR}/src/stubs

SRCS.util=	\
	fontaccel.c \
	fontnames.c \
	fontutil.c \
	fontxlfd.c \
	format.c \
	miscutil.c \
	patcache.c \
	private.c \
	utilbitmap.c
.PATH:	${XFONT2DIR}/src/util

SRCS.fontfile=	\
	bitsource.c \
	bufio.c \
	decompress.c \
	defaults.c \
	dirfile.c \
	fileio.c \
	filewr.c \
	fontdir.c \
	fontencc.c \
	fontfile.c \
	fontscale.c \
	gunzip.c \
	register.c \
	renderers.c \
	catalogue.c \
	bunzip2.c
.PATH:	${XFONT2DIR}/src/fontfile

SRCS.FreeType=	\
	ftenc.c \
	ftfuncs.c \
	fttools.c \
	xttcap.c
.PATH:	${XFONT2DIR}/src/FreeType

SRCS.bitmap=	\
	bitmap.c \
	bitmapfunc.c \
	bitmaputil.c \
	bitscale.c \
	fontink.c \
	bitmap.c \
	bitmapfunc.c \
	bitmaputil.c \
	bitscale.c \
	fontink.c \
	bdfread.c \
	bdfutils.c \
	pcfread.c
SRCS.bitmap+=	\
	pcfwrite.c
# SNF support not enabled
.if 0
SRCS.bitmap+=	\
	snfread.c
.endif
.PATH:	${XFONT2DIR}/src/bitmap

SRCS.builtins=	\
	dir.c \
	file.c \
	fonts.c \
	fpe.c \
	render.c
.PATH:	${XFONT2DIR}/src/builtins

SRCS.fc=	\
	fsconvert.c \
	fserve.c \
	fsio.c \
	fstrans.c
.PATH:	${XFONT2DIR}/src/fc

.for dir in stubs util fontfile FreeType bitmap builtins fc
SRCS+=	${SRCS.${dir}}
.endfor

INCS=	libxfont2.h
INCSDIR=${X11INCDIR}/X11/fonts

CPPFLAGS+=	${X11FLAGS.THREADLIB} ${X11FLAGS.CONNECTION}
CPPFLAGS+=	-DHAS_SNPRINTF -DHAVE_CONFIG_H
CPPFLAGS+=      -DBDFFORMAT -DSNFFORMAT -DPCFFORMAT -DX_GZIP_FONT_COMPRESSION -DX_BZIP2_FONT_COMPRESSION
CPPFLAGS+=	-DFONT_ENCODINGS_DIRECTORY=\"${X11LIBDIR}/X11/fonts/encodings/encodings.dir\" 
CPPFLAGS+=	-I${XFONT2DIR}/src/stubs
CPPFLAGS+=	-I${XFONT2DIR}/include
CPPFLAGS+=	-I${XFONT2DIR}/../include
CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}
CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}/X11/fonts
CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}/freetype2
CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}/freetype2/freetype

CWARNFLAGS.clang+=	-Wno-format

LIBDPLIBS=\
	Xt	${.CURDIR}/../libXt \
	SM	${.CURDIR}/../libSM \
	ICE	${.CURDIR}/../libICE \
	Xext	${.CURDIR}/../libXext \
	fontenc	${.CURDIR}/../libfontenc \
	freetype	${.CURDIR}/../freetype \
	X11	${.CURDIR}/../libX11/dynamic \
	m	${.CURDIR}/../../../../../lib/libm \
	z	${.CURDIR}/../../../../../lib/libz \
	bz2	${.CURDIR}/../../../../../lib/libbz2

PKGDIST=	${LIB}

.include <bsd.x11.mk>
.include <bsd.lib.mk>
