#	$NetBSD: Makefile,v 1.3 2003/09/20 06:20:53 lukem Exp $

XTODO=	SUBDIR=*/module

.include <bsd.own.mk>

LIB=	Xfont
.PATH:	${X11SRCDIR.xc}/lib/font

SRCS.bitmap=	bdfread.c bdfutils.c bitmap.c bitmapfunc.c bitmaputil.c \
		bitscale.c fontink.c pcfread.c pcfwrite.c snfread.c
.PATH:		${X11SRCDIR.xc}/lib/font/bitmap
.for fn in ${SRCS.bitmap}
CPPFLAGS.${fn}=	-I${X11SRCDIR.xc}/include/fonts \
		-I${X11SRCDIR.xc}/lib/font/include \
		-DBDFFORMAT -DPCFFORMAT -DSNFFORMAT
.endfor
CPPFLAGS.bitmapfunc.c+=	-DX_GZIP_FONT_COMPRESSION
CPPFLAGS.bitscale.c+=	-DX_GZIP_FONT_COMPRESSION

SRCS.fontfile=	bitsource.c bufio.c decompress.c defaults.c dirfile.c \
		encparse.c ffcheck.c fileio.c filewr.c fontdir.c \
		fontenc.c fontencc.c fontfile.c fontscale.c gunzip.c \
		printerfont.c register.c renderers.c
.PATH:		${X11SRCDIR.xc}/lib/font/fontfile
.for fn in ${SRCS.fontfile}
CPPFLAGS.${fn}=	-I${X11SRCDIR.xc}/include/fonts \
		-I${X11SRCDIR.xc}/lib/font/include \
		-I${X11SRCDIR.xc}/programs/Xserver/include \
		-DFONTDIRATTRIB -DBUILD_SPEEDO -DBUILD_TYPE1 -DBUILD_CID \
		-DBUILD_FREETYPE -DX_GZIP_FONT_COMPRESSION \
		-DFONT_ENCODINGS_DIRECTORY=\"${X11FONTDIR}/encodings/encodings.dir\"
.endfor

SRCS.fc=	fsconvert.c fserve.c fsio.c transport.c
.PATH:		${X11SRCDIR.xc}/lib/font/fc
.PATH:		${X11SRCDIR.xc}/lib/xtrans
.for fn in ${SRCS.fc}
CPPFLAGS.${fn}=	-I${X11SRCDIR.xc}/include/fonts \
		-I${X11SRCDIR.xc}/lib/font/include \
		${X11FLAGS.CONNECTION} \
		-DFONT_t -DTRANS_CLIENT -DTRANS_SERVER -DTRANS_REOPEN \
		-DBSD44SOCKETS
.endfor

SRCS.fontcache=	fontcache.c
.PATH:		${X11SRCDIR.xc}/lib/font/fontcache
.for fn in ${SRCS.fontcache}
CPPFLAGS.${fn}=	-I${X11SRCDIR.xc}/lib/font/fontcache \
		-I${X11SRCDIR.xc}/include/fonts \
		-I${X11SRCDIR.xc}/lib/font/include \
		-I${X11SRCDIR.xc}/lib/font/fontcache \
		-DFONTCACHE
.endfor

SRCS.Speedo=	bics-unicode.c do_char.c do_trns.c out_bl2d.c \
		out_blk.c out_scrn.c out_util.c reset.c set_spcs.c \
		set_trns.c spencode.c sperr.c spfile.c spfont.c \
		spfuncs.c spglyph.c spinfo.c
.PATH:		${X11SRCDIR.xc}/lib/font/Speedo
.for fn in ${SRCS.Speedo}
CPPFLAGS.${fn}=	-I${X11SRCDIR.xc}/include/fonts \
		-I${X11SRCDIR.xc}/lib/font/include \
		-I${X11SRCDIR.xc}/programs/Xserver/include
.endfor

SRCS.Type1=	afm.c arith.c cidchar.c curves.c fontfcn.c hints.c \
		lines.c objects.c paths.c regions.c scanfont.c \
		spaces.c t1funcs.c t1info.c t1io.c t1malloc.c \
		t1snap.c t1stub.c t1unicode.c token.c Type1-type1.c util.c
.for fn in ${SRCS.Type1}
CPPFLAGS.${fn}=	-I${X11SRCDIR.xc}/include/fonts \
		-I${X11SRCDIR.xc}/lib/font/include \
		-I${DESTDIR}/${X11INCDIR}/X11 \
		-I${X11SRCDIR.xc}/programs/Xserver/include \
		-DBUILDCID -DHAVE_CFM -DUSE_MMAP
.endfor
BUILDSYMLINKS+=	${X11SRCDIR.xc}/lib/font/Type1/type1.c Type1-type1.c
CPPFLAGS.Type1-type1.c+=	-I${X11SRCDIR.xc}/lib/font/Type1

SRCS.FreeType=	autohint.c cff.c ftbase.c ftenc.c ftfuncs.c ftglyph.c \
		ftinit.c ftsystem.c fttools.c fttype1.c ftxf86.c \
		psaux.c pshinter.c psnames.c raster.c sfnt.c \
		truetype.c FreeType-type1.c
.PATH:		${X11SRCDIR.xc}/lib/font/FreeType
.PATH:		${X11SRCDIR.xc}/lib/font/Type1
FREETYPESRC=	${X11SRCDIR.xc}/extras/freetype2/src
.PATH:		${FREETYPESRC}/autohint
.PATH:		${FREETYPESRC}/base
.PATH:		${FREETYPESRC}/cff
.PATH:		${FREETYPESRC}/psaux
.PATH:		${FREETYPESRC}/pshinter
.PATH:		${FREETYPESRC}/psnames
.PATH:		${FREETYPESRC}/raster
.PATH:		${FREETYPESRC}/sfnt
.PATH:		${FREETYPESRC}/truetype
.for fn in ${SRCS.FreeType}
CPPFLAGS.${fn}=	-I${X11SRCDIR.xc}/lib/font/FreeType \
		-I${X11SRCDIR.xc}/include/fonts \
		-I${X11SRCDIR.xc}/lib/font/include \
		-I${DESTDIR}/${X11INCDIR}/X11 \
		-I${X11SRCDIR.xc}/programs/Xserver/include \
		-I${DESTDIR}/${X11INCDIR}/freetype2 \
		-DGCCUSESGAS -DAVOID_GLYPHBLT -DPIXPRIV -DSINGLEDEPTH \
		-DXFreeXDGA -DXvExtension -DXFree86LOADER  -DXFree86Server \
		-DXF86VIDMODE -DXvMCExtension  -DSMART_SCHEDULE -DBUILDDEBUG \
		-DXResExtension
.endfor
BUILDSYMLINKS+=	${X11SRCDIR.xc}/extras/freetype2/src/type1/type1.c FreeType-type1.c
CPPFLAGS.FreeType-type1.c+=	-I${X11SRCDIR.xc}/extras/freetype2/src/type1

SRCS.util=	atom.c fontaccel.c fontnames.c fontutil.c fontxlfd.c \
		format.c miscutil.c patcache.c private.c utilbitmap.c
.PATH:		${X11SRCDIR.xc}/lib/font/util
.for fn in ${SRCS.util}
CPPFLAGS.${fn}=	-I${X11SRCDIR.xc}/include/fonts \
		-I${X11SRCDIR.xc}/lib/font/include
.endfor

SRCS=	${SRCS.bitmap} ${SRCS.fontfile} ${SRCS.fc} ${SRCS.fontcache} \
	${SRCS.Speedo} ${SRCS.Type1} ${SRCS.FreeType} ${SRCS.util}


SUBDIR=	stubs

.include <bsd.x11.mk>
.include <bsd.lib.mk>
.include <bsd.subdir.mk>
