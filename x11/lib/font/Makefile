#	$NetBSD: Makefile,v 1.8 2004/04/05 12:03:25 lukem Exp $

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
#.PATH:		${X11SRCDIR.xc}/extras/freetype2/src/base
#SRCS.FreeType+=	ftbase.c ftinit.c ftglyph.c ftbbox.c ftsystem.c fttype1.c ftxf86.c ftbdf.c
#.PATH:		${X11SRCDIR.xc}/extras/freetype2/src/bdf
#SRCS.FreeType+=	bdfdrivr.c bdflib.c
#.PATH:		${X11SRCDIR.xc}/extras/freetype2/src/raster
#SRCS.FreeType+=	ftrend1.c ftraster.c
#.PATH:		${X11SRCDIR.xc}/extras/freetype2/src/smooth
#SRCS.FreeType+=	ftsmooth.c ftgrays.c
#.PATH:		${X11SRCDIR.xc}/extras/freetype2/src/truetype
#SRCS.FreeType+=	ttdriver.c ttobjs.c ttpload.c
#.PATH:		${X11SRCDIR.xc}/extras/freetype2/src/psaux
#SRCS.FreeType+=	psauxmod.c psobjs.c t1cmap.c t1decode.c
#.PATH:		${X11SRCDIR.xc}/extras/freetype2/src/type1
#SRCS.FreeType+=	t1driver.c
#.PATH:		${X11SRCDIR.xc}/extras/freetype2/src/cid
#SRCS.FreeType+=	cidriver.c cidgload.c cidobjs.c cidload.c cidparse.c
#.PATH:		${X11SRCDIR.xc}/extras/freetype2/src/type42
#SRCS.FreeType+=	t42drivr.c
#.PATH:		${X11SRCDIR.xc}/extras/freetype2/src/psnames
#SRCS.FreeType+=	psmodule.c
#.PATH:		${X11SRCDIR.xc}/extras/freetype2/src/sfnt
#SRCS.FreeType+=	sfdriver.c ttload.c ttpost.c ttsbit.c ttcmap.c sfobjs.c ttcmap0.c
#.PATH:		${X11SRCDIR.xc}/extras/freetype2/src/winfonts
#SRCS.FreeType+=	winfnt.c
#.PATH:		${X11SRCDIR.xc}/extras/freetype2/src/autohint
#SRCS.FreeType+=	ahmodule.c ahhint.c ahglyph.c ahangles.c ahglobal.c
#.PATH:		${X11SRCDIR.xc}/extras/freetype2/src/cff
#SRCS.FreeType+=	cffdrivr.c cffobjs.c cffload.c cffparse.c cffcmap.c cffgload.c
#.PATH:		${X11SRCDIR.xc}/extras/freetype2/src/pfr
#SRCS.FreeType+=	pfrdrivr.c pfrobjs.c pfrload.c pfrgload.c pfrcmap.c
#.PATH:		${X11SRCDIR.xc}/extras/freetype2/src/pcf
#SRCS.FreeType+=	pcfdriver.c pcfutil.c
.for fn in ${SRCS.FreeType}
CPPFLAGS.${fn}=	-I${DESTDIR}${X11INCDIR}/freetype2 \
		-I${X11SRCDIR.xc}/include/fonts \
		-I${X11SRCDIR.xc}/lib/font/include \
		-I${DESTDIR}${X11INCDIR}/X11 \
		-I${X11SRCDIR.xc}/programs/Xserver/include \
		-I${X11SRCDIR.xc}/extras/freetype2/src/truetype \
		-I${X11SRCDIR.xc}/extras/freetype2/src/pcf 
CPPFLAGS.${fn}+=${X11FLAGS.SERVER} -DXFreeXDGA -DXFree86LOADER \
		-DXF86VIDMODE -DXFREE86_FT2
.endfor


# X-TrueType
# .PATH:		${X11SRCDIR.xc}/lib/font/X-TrueType
# .PATH:		${X11SRCDIR.xc}/extras/FreeType/lib/arch/unix
# SRCS.X-TrueType=xttprop.c xttfuncs.c xttcconv.c xttcap.c xttmisc.c \
#		xttblit.c freetype.c ftxcmap.c ftxsbit.c
#.for fn in ${SRCS.X-TrueType}
#CPPFLAGS.${fn}=	-DTT_MAKE_OPTION_SINGLE_OBJECT=1 \
#		-DTT_CONFIG_OPTION_EXTEND_ENGINE \
#		-DCCONV_MODULE -DCCONV_MODULE_DIR=\"${X11USRLIBDIR}/modules\" \
#		-DCCONV_MODULE_SUBDIR=\"${X11USRLIBDIR}/modules/codeconv\" \
#		-DOPT_ENCODINGS -DFREETYPE1_BUILD_DEFINES
# .endfor


# builtins
#.PATH:		${X11SRCDIR.xc}/lib/font/builtins
#SRCS.builtins= 	dir.c file.c fonts.c fpe.c render.c
#.for fn in ${SRCS.builtins}
#CPPFLAGS.${fn}=	-I${X11SRCDIR.xc}/lib/font/include \
#		-I${X11SRCDIR.xc}/include/fonts
#.endfor


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

SUBDIR=	stubs xtt

.include <bsd.x11.mk>
.include <bsd.lib.mk>
.include <bsd.subdir.mk>

