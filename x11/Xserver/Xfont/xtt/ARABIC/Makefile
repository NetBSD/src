#	$NetBSD: Makefile,v 1.1 2004/01/06 06:46:51 rtr Exp $

XMODULEDIR=	${X11USRLIBDIR}/modules/codeconv

.include "../../../Makefile.serverlib"

.include <bsd.own.mk>

LIB=	ARABIC
.PATH:	${X11SRCDIR.xc}/extras/X-TrueType/ARABIC

SRCS=	main.c MULEARABICtoUCS2.c CP1256toUCS2.c XATERMtoUCS2.c

CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}/X11/fonts/codeconv \
		-I${X11SRCDIR.xc}/extras/FreeType/lib \
		-I${X11SRCDIR.xc}/extras/FreeType/lib/extend

.include <bsd.x11.mk>
LIBDIR=	${XMODULEDIR}
.include <bsd.lib.mk>
