#	$NetBSD: Makefile,v 1.3 2003/09/13 20:18:56 lukem Exp $

NOPIC=	1

.include <bsd.own.mk>

LIB=	fontenc
.PATH:	${X11SRCDIR.xc}/lib/font/fontfile

SRCS=	encparse.c fontenc.c

INCS=	fontenc.h
INCSDIR=${X11INCDIR}/X11/fonts
.PATH:	${X11SRCDIR.xc}/lib/font/include

CPPFLAGS+=	-DFONTENC_NO_LIBFONT
CPPFLAGS+=	-DFONT_ENCODINGS_DIRECTORY=\"${X11FONTDIR}/encodings/encodings.dir\"
CPPFLAGS+=	-I${DESTDIR}/${X11INCDIR}/X11/fonts

LDADD+=	-lz
DPADD+=	${LIBZ}

.include <bsd.x11.mk>
.include <bsd.lib.mk>
