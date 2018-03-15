#	$NetBSD: Makefile,v 1.6.26.1 2018/03/15 09:12:00 pgoyette Exp $

NOMAN=		1

.include <bsd.own.mk>

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
