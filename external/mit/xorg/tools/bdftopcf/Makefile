#	$NetBSD: Makefile,v 1.8 2018/05/01 17:42:00 christos Exp $

.include <bsd.init.mk>

HOSTPROG=	bdftopcf
.PATH:		${X11SRCDIR.bdftopcf}

FREETYPE=       ${X11SRCDIR.freetype}

SRCS=		atom.c \
		bdfread.c \
		bdfutils.c \
		bitmap.c \
		bitmaputil.c \
		bufio.c \
		defaults.c \
		fileio.c \
		filewr.c \
		fontaccel.c \
		fontink.c \
		pcfwrite.c \
		private.c \
		utilbitmap.c \
		bdftopcf.c

HOST_CPPFLAGS+=	-I${DESTDIR}${X11INCDIR} \
		-I${DESTDIR}${X11INCDIR} \
		-I${DESTDIR}${X11INCDIR}/freetype2 \
		-DBDFFORMAT -DPCFFORMAT -DSNFFORMAT -DX_GZIP_FONT_COMPRESSION \
		-DFONT_ENCODINGS_DIRECTORY=\"${X11FONTDIR}/encodings/encodings.dir\" \
		-DPACKAGE_STRING="\"NetBSD tool bdftopcf 1.1\""

LDADD+=		-lz

.include <bsd.x11.mk>
.include <bsd.hostprog.mk>
