#	$NetBSD: Makefile,v 1.5 2022/10/28 09:43:59 wiz Exp $

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
CPPFLAGS+=	-DHAVE_REALLOCARRAY

LIBDPLIBS+=	z ${NETBSDSRCDIR}/lib/libz

PKGCONFIG=	fontenc

.include <bsd.x11.mk>
.include <bsd.lib.mk>
