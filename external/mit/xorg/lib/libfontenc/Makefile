#	$NetBSD: Makefile,v 1.3 2016/01/05 13:07:46 christos Exp $

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

LIBDPLIBS+=	z ${NETBSDSRCDIR}/lib/libz

PKGCONFIG=	fontenc

.include <bsd.x11.mk>
.include <bsd.lib.mk>
