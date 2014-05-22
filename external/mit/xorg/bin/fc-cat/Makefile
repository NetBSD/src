#	$NetBSD: Makefile,v 1.1.1.1.4.1 2014/05/22 14:09:35 yamt Exp $

.include <bsd.own.mk>

PROG=	fc-cat

CPPFLAGS+=	-I${X11SRCDIR.fontconfig} -I.

LDADD+=	-lfontconfig -lfreetype -lexpat
DPADD+=	${LIBFONTCONFIG} ${LIBFREETYPE} ${LIBEXPAT}

.PATH:	${X11SRCDIR.fontconfig}/${PROG}

.include "../../lib/fontconfig/src/Makefile.fcarch"

.include <bsd.x11.mk>
.include <bsd.prog.mk>
