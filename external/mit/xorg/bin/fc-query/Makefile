#	$NetBSD: Makefile,v 1.3 2016/04/17 02:16:26 nakayama Exp $

.include <bsd.own.mk>

PROG=	fc-query

CPPFLAGS+=	-I${X11SRCDIR.fontconfig} -I. -I${DESTDIR}${X11INCDIR}/freetype2

LDADD+=	-lfontconfig -lfreetype -lexpat
DPADD+=	${LIBFONTCONFIG} ${LIBFREETYPE} ${LIBEXPAT}

.PATH:	${X11SRCDIR.fontconfig}/${PROG}

.include "../../lib/fontconfig/src/Makefile.fcarch"

.include <bsd.x11.mk>
.include <bsd.prog.mk>
