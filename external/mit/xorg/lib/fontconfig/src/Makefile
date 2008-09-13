#	$NetBSD: Makefile,v 1.2 2008/09/13 19:57:23 cube Exp $

.include <bsd.own.mk>

LIB=	fontconfig

.PATH:	${X11SRCDIR.${LIB}}/src
SRCS=	fcatomic.c fcblanks.c fccache.c fccfg.c fccharset.c fcdbg.c \
	fcdefault.c fcdir.c fcfreetype.c fcfs.c fcinit.c fclang.c \
	fclist.c fcmatch.c fcmatrix.c fcname.c fcpat.c fcserialize.c \
	fcstr.c fcxml.c ftglue.h ftglue.c

LIBDPLIBS=\
	expat		${.CURDIR}/../../expat \
	freetype	${.CURDIR}/../../freetype

# XXX FONTCONFIG_PATH was arbitrarily chosen
CPPFLAGS+=	-DFONTCONFIG_PATH='"/etc/X11/fonts"' \
		-DFC_DEFAULT_FONTS='"${X11FONTDIR}/Type1"' \
		-DFC_CACHEDIR='"/var/cache/fontconfig"' \
		-DHAVE_FT_BITMAP_SIZE_Y_PPEM \
		-DHAVE_FT_GET_BDF_PROPERTY \
		-DHAVE_FT_GET_NEXT_CHAR \
		-DHAVE_FT_GET_PS_FONT_INFO \
		-DHAVE_FT_GET_X11_FONT_FORMAT \
		-DHAVE_FT_HAS_PS_GLYPH_NAMES \
		-DHAVE_EXPAT -DXFREE86_FT2 -DHAVE_INTTYPES_H \
		-DHAVE_STDINT_H -DHAVE_RANDOM

CPPFLAGS+=	${X11FLAGS.THREADLIB}
CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}/freetype2 \
		-I${X11SRCDIR.freetype}/src \
		-I${X11SRCDIR.freetype}/include \
		-I${X11SRCDIR.${LIB}}/../include \
		-I.

FCARCH_DEPFILE=	fccache.c
.include "Makefile.fcarch"

CLEANFILES+= fcarch.h

.include <bsd.x11.mk>
.include <bsd.lib.mk>
