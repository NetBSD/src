#	$NetBSD: Makefile,v 1.2.2.3 2008/09/24 16:35:06 wrstuden Exp $

.include <bsd.own.mk>

LIB=	fontenc
.PATH:	${X11SRCDIR.${LIB}}/src

SRCS=	encparse.c fontenc.c

INCS=	fontenc.h
INCSDIR=${X11INCDIR}/X11/fonts
.PATH:	${X11SRCDIR.${LIB}}/include/X11/fonts

CPPFLAGS+=	-DFONTENC_NO_LIBFONT
CPPFLAGS+=	-DFONT_ENCODINGS_DIRECTORY=\"${X11FONTDIR}/encodings/encodings.dir\"
CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}/X11/fonts

LDADD+=	-lz
DPADD+=	${LIBZ}

PKGCONFIG=	fontenc

.include <bsd.x11.mk>
.include <bsd.lib.mk>
