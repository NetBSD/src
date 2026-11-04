#	$NetBSD: Makefile,v 1.4.12.1 2026/11/04 23:15:23 perseant Exp $

.include <bsd.own.mk>

PROG=	fonttosfnt
SRCS=	fonttosfnt.c read.c write.c struct.c util.c

# XXX XVENDORNAME and XVENDORNAMESHORT should live in a common location
CPPFLAGS+=	-DXFREE86_FT2 -I${DESTDIR}${X11INCDIR}/freetype2 \
		-I${DESTDIR}${X11INCDIR}/freetype2/freetype \
		-DXVENDORNAMESHORT=${XVENDORNAMESHORT} \
		-DXVENDORNAME=${XVENDORNAME}
CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}/X11/fonts


LDADD+=	-lfreetype -lfontenc -lz -lm
DPADD+=	 ${LIBFREETYPE} ${LIBFONTENC} ${LIBZ} ${LIBM}

.PATH:	${X11SRCDIR.${PROG}} ${X11SRCDIR.${PROG}}/man

CWARNFLAGS.clang+=	-Wno-format

.include "../../xorg-pkg-ver.mk"

.include <bsd.x11.mk>
.include <bsd.prog.mk>
