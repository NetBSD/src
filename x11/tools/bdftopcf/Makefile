#	$NetBSD: Makefile,v 1.2 2003/09/13 20:19:08 lukem Exp $

NOMAN=		1

.include <bsd.own.mk>

HOSTPROG=	bdftopcf
.PATH:		${X11SRCDIR.xc}/programs/${HOSTPROG}

SRCS=		bdftopcf.c

HOST_CPPFLAGS+=	-I${DESTDIR}/${X11INCDIR} \
		-I${X11SRCDIR.xc}/include/fonts \
		-I${X11SRCDIR.xc}/lib/font/include \
		-I${X11SRCDIR.xc}/lib/font/bitmap \
		-I${X11SRCDIR.xc}/programs/Xserver/include \
		-DBDFFORMAT -DPCFFORMAT -DSNFFORMAT -DX_GZIP_FONT_COMPRESSION \
		-DFONT_ENCODINGS_DIRECTORY=\"${X11FONTDIR}/encodings/encodings.dir\"

.PATH:		${X11SRCDIR.xc}/lib/font/bitmap
SRCS+=		bdfread.c bdfutils.c bitmap.c bitmaputil.c fontink.c \
		pcfread.c pcfwrite.c

.PATH:		${X11SRCDIR.xc}/lib/font/fontfile
SRCS+=		bufio.c decompress.c defaults.c fileio.c filewr.c gunzip.c

.PATH:		${X11SRCDIR.xc}/lib/font/util
SRCS+=		atom.c fontaccel.c miscutil.c private.c utilbitmap.c

LDADD+=		-lz

.include <bsd.x11.mk>
.include <bsd.hostprog.mk>
