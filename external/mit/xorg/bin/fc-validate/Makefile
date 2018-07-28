#	$NetBSD: Makefile,v 1.1.32.1 2018/07/28 04:37:21 pgoyette Exp $

.include <bsd.own.mk>

PROG=	fc-validate

CPPFLAGS+=	-I${X11SRCDIR.fontconfig} -I. -I${DESTDIR}${X11INCDIR}/freetype2
CPPFLAGS+=	-DHAVE_GETOPT_LONG=1

LDADD+=	-lfontconfig -lfreetype -lexpat
DPADD+=	${LIBFONTCONFIG} ${LIBFREETYPE} ${LIBEXPAT}

.PATH:	${X11SRCDIR.fontconfig}/${PROG}

.include "../../lib/fontconfig/src/Makefile.fcarch"

.include <bsd.x11.mk>
.include <bsd.prog.mk>
