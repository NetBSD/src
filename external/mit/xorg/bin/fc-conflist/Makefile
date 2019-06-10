#	$NetBSD: Makefile,v 1.1.2.2 2019/06/10 22:02:34 christos Exp $

.include <bsd.own.mk>

PROG=	fc-conflist

CPPFLAGS+=	-I${X11SRCDIR.fontconfig} -I.
CPPFLAGS+=	-DHAVE_GETOPT_LONG=1

LDADD+=	-lfontconfig -lfreetype -lexpat
DPADD+=	${LIBFONTCONFIG} ${LIBFREETYPE} ${LIBEXPAT}

.PATH:	${X11SRCDIR.fontconfig}/${PROG}

.include "../../lib/fontconfig/src/Makefile.fcarch"

.include <bsd.x11.mk>
.include <bsd.prog.mk>
