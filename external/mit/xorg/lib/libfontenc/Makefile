#	$NetBSD: Makefile,v 1.6 2026/07/12 00:10:54 mrg Exp $

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
PKGCONFIG_SED_FLAGS= \
	-e "s,@Z_REQUIRES@,-lz,"

.include <bsd.x11.mk>
.include <bsd.lib.mk>
