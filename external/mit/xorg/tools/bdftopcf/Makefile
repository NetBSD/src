#	$NetBSD: Makefile,v 1.3 2009/06/10 07:52:07 mrg Exp $

NOMAN=		1

.include <bsd.own.mk>

HOSTPROG=	bdftopcf
.PATH:		${X11SRCDIR.bdftopcf}

FREETYPE=       ${X11SRCDIR.freetype}

SRCS=		bdftopcf.c

HOST_CPPFLAGS+=	-I${DESTDIR}${X11INCDIR} \
		-I${DESTDIR}${X11INCDIR} \
		-I${DESTDIR}${X11INCDIR}/X11/fonts \
		-I${DESTDIR}${X11INCDIR}/freetype2 \
		-I${X11SRCDIR.Xfont}/src/stubs \
		-DBDFFORMAT -DPCFFORMAT -DSNFFORMAT -DX_GZIP_FONT_COMPRESSION -DX_BZIP2_FONT_COMPRESSION \
		-DFONT_ENCODINGS_DIRECTORY=\"${X11FONTDIR}/encodings/encodings.dir\"

.PATH:		${X11SRCDIR.Xfont}/src/bitmap
SRCS+=		bdfread.c bdfutils.c bitmap.c bitmaputil.c fontink.c \
		pcfread.c pcfwrite.c

.PATH:		${X11SRCDIR.Xfont}/src/fontfile
SRCS+=		bufio.c decompress.c defaults.c fileio.c filewr.c gunzip.c bunzip2.c

.PATH:		${X11SRCDIR.Xfont}/src/util
SRCS+=		atom.c fontaccel.c miscutil.c private.c utilbitmap.c

LDADD+=		-lz -lbz2

.include <bsd.x11.mk>
.include <bsd.hostprog.mk>
