#	$NetBSD: Makefile,v 1.8.2.1 2004/11/15 09:12:46 rtr Exp $

.include <bsd.own.mk>

LIB=	Xfont
#.PATH:	${X11SRCDIR.xc}/lib/font

# bitmap
.PATH:		${X11SRCDIR.xc}/lib/font/bitmap
SRCS.bitmap=	bdfread.c bdfutils.c bitmap.c bitmaputil.c bitscale.c \
		bitmapfunc.c pcfread.c pcfwrite.c snfread.c fontink.c
.for fn in ${SRCS.bitmap}
CPPFLAGS.${fn}=	-I${X11SRCDIR.xc}/lib/font/include \
		-I${X11SRCDIR.xc}/include/fonts
CPPFLAGS.${fn}+=-DBDFFORMAT -DPCFFORMAT -DSNFFORMAT
.endfor
CPPFLAGS.bitmapfunc.c+=	-DX_GZIP_FONT_COMPRESSION
CPPFLAGS.bitscale.c+=	-DX_GZIP_FONT_COMPRESSION


# fontfile
.PATH:		${X11SRCDIR.xc}/lib/font/fontfile
SRCS.fontfile=	dirfile.c fontdir.c fontfile.c fileio.c fontscale.c \
		defaults.c bitsource.c register.c renderers.c bufio.c \
		decompress.c filewr.c printerfont.c ffcheck.c \
		fontenc.c encparse.c fontencc.c gunzip.c
.for fn in ${SRCS.fontfile}
CPPFLAGS.${fn}=	-I${X11SRCDIR.xc}/lib/font/include \
		-I${X11SRCDIR.xc}/include/fonts \
		-I${X11SRCDIR.xc}/programs/Xserver/include
CPPFLAGS.${fn}+=-DFONTDIRATTRIB -DBUILD_SPEEDO -DBUILD_TYPE1 \
		-DBUILD_CID -DBUILD_FREETYPE -DX_GZIP_FONT_COMPRESSION \
                -DFONT_ENCODINGS_DIRECTORY=\"${X11FONTDIR}/encodings/encodings.dir\"
.endfor


# fc
.PATH:		${X11SRCDIR.xc}/lib/font/fc
.PATH:		${X11SRCDIR.xc}/lib/xtrans
SRCS.fc=	fsconvert.c fserve.c fsio.c transport.c
.for fn in ${SRCS.fc}
CPPFLAGS.${fn}=	-I${DESTDIR}${X11INCDIR}/X11/fonts \
		-I${X11SRCDIR.xc}/lib/font/include \
		-I${X11SRCDIR.xc}/lib/xtrans \
		-I${X11SRCDIR.xc}/include/fonts
CPPFLAGS.${fn}+=${X11FLAGS.CONNECTION} -DFONT_t -DTRANS_CLIENT \
		-DTRANS_SERVER -DTRANS_REOPEN -DBSD44SOCKETS
.endfor

.PATH:		${X11SRCDIR.xc}/lib/font/fontcache
SRCS.fontcache=	fontcache.c
CPPFLAGS.fontcache.c=	-DFONTCACHE
CPPFLAGS.fontcache.c+=	-I${X11SRCDIR.xc}/lib/font/include \
			-I${X11SRCDIR.xc}/include/fonts


# Speedo
.PATH:		${X11SRCDIR.xc}/lib/font/Speedo
SRCS.Speedo=	spfuncs.c spfile.c spinfo.c sperr.c spfont.c spglyph.c \
		spencode.c bics-unicode.c do_char.c do_trns.c \
		out_bl2d.c out_blk.c out_scrn.c out_util.c reset.c \
		set_spcs.c set_trns.c
.for fn in ${SRCS.Speedo}
CPPFLAGS.${fn}=	-I${X11SRCDIR.xc}/lib/font/include \
		-I${X11SRCDIR.xc}/include/fonts \
		-I${X11SRCDIR.xc}/programs/Xserver/include
.endfor


# Type1
.PATH:		${X11SRCDIR.xc}/lib/font/Type1
SRCS.Type1=	arith.c curves.c fontfcn.c hints.c lines.c objects.c \
		paths.c regions.c scanfont.c spaces.c t1funcs.c \
		t1info.c t1io.c t1malloc.c t1snap.c t1stub.c token.c \
		type1.c util.c cidchar.c afm.c t1unicode.c
.for fn in ${SRCS.Type1}
CPPFLAGS.${fn}=	-I${X11SRCDIR.xc}/lib/font/include \
		-I${X11SRCDIR.xc}/include/fonts \
		-I${X11SRCDIR.xc}/programs/Xserver/include \
		-I${DESTDIR}${X11INCDIR}/X11
CPPFLAGS.${fn}+=-DBUILDCID -DHAVE_CFM -DUSE_MMAP
.endfor


# FreeType
.PATH:		${X11SRCDIR.xc}/lib/font/FreeType
SRCS.FreeType=	xttcap.c ftfuncs.c ftenc.c fttools.c

.for fn in ${SRCS.FreeType}
CPPFLAGS.${fn}=	-I${DESTDIR}${X11INCDIR}/freetype2 \
		-I${X11SRCDIR.xc}/include/fonts \
		-I${X11SRCDIR.xc}/lib/font/include \
		-I${DESTDIR}${X11INCDIR}/X11 \
		-I${X11SRCDIR.xc}/programs/Xserver/include \
		-I${X11SRCDIR.xc}/extras/freetype2/src/truetype \
		-I${X11SRCDIR.xc}/extras/freetype2/include \
		-I${X11SRCDIR.xc}/extras/freetype2/src/pcf 
CPPFLAGS.${fn}+=${X11FLAGS.SERVER} -DXFreeXDGA -DXFree86LOADER \
		-DXF86VIDMODE -DXFREE86_FT2
.endfor

# util
.PATH:		${X11SRCDIR.xc}/lib/font/util
SRCS.util=	utilbitmap.c fontnames.c fontutil.c fontxlfd.c \
		format.c fontaccel.c atom.c miscutil.c private.c \
		patcache.c
.for fn in ${SRCS.util}
CPPFLAGS.${fn}=	-I${X11SRCDIR.xc}/lib/font/include \
		-I${X11SRCDIR.xc}/include/fonts
.endfor

SRCS=		${SRCS.bitmap} ${SRCS.fontfile} ${SRCS.fc} \
		${SRCS.fontcache} ${SRCS.Speedo} ${SRCS.Type1} \
		${SRCS.FreeType} ${SRCS.X-TrueType} ${SRCS.builtins} \
		${SRCS.util}

SUBDIR=	stubs

.include <bsd.x11.mk>
.include <bsd.lib.mk>
.include <bsd.subdir.mk>

