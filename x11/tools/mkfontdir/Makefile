#	$NetBSD: Makefile,v 1.2 2003/09/13 20:19:09 lukem Exp $

NOMAN=		1

.include <bsd.own.mk>

HOSTPROG=	mkfontdir
.PATH:		${X11SRCDIR.xc}/programs/${HOSTPROG}

SRCS=		mkfontdir.c

HOST_CPPFLAGS+=	-I${DESTDIR}/${X11INCDIR} \
		-I${X11SRCDIR.xc}/include/fonts \
		-I${X11SRCDIR.xc}/lib/font/include \
		-I${X11SRCDIR.xc}/programs/Xserver/include \
		-DBDFFORMAT -DPCFFORMAT -DSNFFORMAT -DX_GZIP_FONT_COMPRESSION \
		-DFONT_ENCODINGS_DIRECTORY=\"${X11FONTDIR}/encodings/encodings.dir\"

.PATH:		${X11SRCDIR.xc}/lib/font/bitmap
SRCS+=		bdfread.c bdfutils.c bitmap.c bitmapfunc.c bitmaputil.c \
		bitscale.c fontink.c pcfread.c snfread.c

.PATH:		${X11SRCDIR.xc}/lib/font/fontfile
SRCS+=		bitsource.c bufio.c decompress.c defaults.c dirfile.c \
		encparse.c fileio.c fontdir.c fontenc.c fontfile.c \
		fontscale.c gunzip.c renderers.c

.PATH:		${X11SRCDIR.xc}/lib/font/util
SRCS+=		atom.c fontaccel.c fontnames.c fontutil.c fontxlfd.c \
		format.c miscutil.c private.c utilbitmap.c

.PATH:		${X11SRCDIR.xc}/lib/font/stubs
SRCS+=		regfpefunc.c

LDADD+=		-lm -lz

.include <bsd.x11.mk>
.include <bsd.hostprog.mk>
