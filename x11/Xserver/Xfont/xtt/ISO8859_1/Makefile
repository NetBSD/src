#	$NetBSD: Makefile,v 1.1 2004/01/06 06:46:51 rtr Exp $

XMODULEDIR=	${X11USRLIBDIR}/modules/codeconv

.include "../../../Makefile.serverlib"

.include <bsd.own.mk>

LIB=	ISO8859_1
.PATH:	${X11SRCDIR.xc}/extras/X-TrueType/ISO8859.1

SRCS=	main.c ISO8859_1toAROMAN.c

CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}/X11/fonts/codeconv \
		-I${X11SRCDIR.xc}/extras/FreeType/lib \
		-I${X11SRCDIR.xc}/extras/FreeType/lib/extend

.include <bsd.x11.mk>
LIBDIR=	${XMODULEDIR}
.include <bsd.lib.mk>
